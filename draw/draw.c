#include "internal.h"

static rect __draw_calc_inner_rect(r_context* ctx, int style, rect outer);
static rect __draw_calc_outer_rect(r_context* ctx, int style, rect* inner);
static rect __draw_measure_text   (r_context* ctx, int style, const char* text, int len, int wrap_width);
static rect __draw_measure_icon   (r_context* ctx, int style, int icon, int* gap);
static int  __draw_break_text     (r_context* ctx, int style, const char* text, int len, int width, bool force, int* w);
static void __draw_get_layout     (r_context* ctx, int style, int* size, int* spacing);

static const rect no_clip = { INT32_MIN / 2, INT32_MIN / 2, INT32_MAX, INT32_MAX };

void r_draw_init(r_draw* c, r_context* ctx, const r_draw_backend* backend, float scale)
{
    *c = (r_draw){0};
    c->context = ctx;
    c->backend = *backend;
    c->scale = scale > 0.0f ? scale : 1.0f;
    c->clip_px = no_clip;
    c->area = no_clip;
    hash_init(&c->style_rules, 64, sizeof(r_draw_style_rule));

    ctx->userptr = c;
    ctx->interface.calc_inner_rect = __draw_calc_inner_rect;
    ctx->interface.calc_outer_rect = __draw_calc_outer_rect;
    ctx->interface.measure_text    = __draw_measure_text;
    ctx->interface.measure_icon    = __draw_measure_icon;
    ctx->interface.break_text      = __draw_break_text;
    ctx->interface.get_layout      = __draw_get_layout;
}

void r_draw_free(r_draw* c)
{
    for(int i = 0; i < RCUT_DRAW_FONT_HASHMAP_SIZE; i++)
        if(c->fonts[i].font) __draw_face_inst_free(c->fonts[i].font);

    for(int i = 0; i < COUNTOF(c->base_fonts); i++)
    {
        if(c->base_fonts[i].face) __draw_face_free(c->base_fonts[i].face);
        free(c->base_fonts[i].path);
    }

    __draw_face_free_state(c);
    __draw_free_text(c);
    free(c->quads);
    free(c->theme);
    free(c->style_rules.keys); free(c->style_rules.values);
    *c = (r_draw){0};
}

void r_draw_set_scale(r_draw* c, float scale)
{
    if(scale <= 0.0f) scale = 1.0f;
    if(scale == c->scale) return;
    c->scale = scale;

    __draw_rescale_fonts(c);
    r_draw_clear_cache(c);
    for(int i = 1; i < RCUT_DRAW_MAX_STYLEBOXES; i++)
    {
        struct r_draw_stylebox* sb = &c->styleboxes[i];
        if(!sb->w || sb->permanent || sb->bg_permanent) continue;
        r_draw_stylebox_def d = sb->definition;
        r_draw_generate_stylebox(c, i, &d);
    }
    for(r_ui_list* l = c->context->chain_start; l; l = l->chain_next)
        l->needs_redraw = true;
}

void __draw_invalidate(r_draw* c, uint64_t* handle)
{
    if(*handle && c->backend.invalidate_cache) c->backend.invalidate_cache(c->backend.userptr, *handle);
    *handle = 0;
}

bool __draw_still_cached(r_draw* c, uint64_t* handle)
{
    if(*handle && c->backend.is_cached(c->backend.userptr, *handle)) return true;
    *handle = 0;
    return false;
}

uint64_t __draw_cached(r_draw* c, uint64_t* handle, const uint32_t* pixels, int w, int h)
{
    if(__draw_still_cached(c, handle)) return *handle;
    if(c->cache_full || !pixels || w <= 0 || h <= 0) return 0;
    *handle = c->backend.cache_argb(c->backend.userptr, w, h, pixels, w * 4, false);
    if(!*handle) c->cache_full = true;
    return *handle;
}

void r_image_invalidate(r_draw* c, r_image_buffer* img)
{
    __draw_invalidate(c, &img->handle);
    if(img->upload == R_IMAGE_UPLOAD_FINISHED) img->upload = R_IMAGE_UPLOAD_ONCE;
}

static void render_image(r_draw* c, r_image_buffer* img, rect r)
{
    if(!img || !img->pixels || img->w <= 0 || img->h <= 0 || !c->backend.cache_image_buffer) return;
    if(img->upload != R_IMAGE_UPLOAD_FINISHED || !__draw_still_cached(c, &img->handle))
    {
        img->handle = c->backend.cache_image_buffer(c->backend.userptr, img);
        if(!img->handle) return;
        if(img->upload == R_IMAGE_UPLOAD_ONCE) img->upload = R_IMAGE_UPLOAD_FINISHED;
    }
    rect px = __draw_to_pixels(r, c->scale);
    __draw_emit(c, img->handle, 0xFFFFFFFF, (r_frect){ 0, 0, img->w, img->h }, (r_frect){ px.x, px.y, px.w, px.h }, c->clip_px);
}

void r_draw_clear_cache(r_draw* c)
{
    for(int i = 0; i < RCUT_DRAW_MAX_STYLEBOXES; i++)
    {
        if(!c->styleboxes[i].permanent)    __draw_invalidate(c, &c->styleboxes[i].handle);
        if(!c->styleboxes[i].bg_permanent) __draw_invalidate(c, &c->styleboxes[i].bg_handle);
    }
    for(int i = 0; i < c->num_sized_styleboxes; i++)
        __draw_invalidate(c, &c->sized_styleboxes[i].handle);
    c->num_sized_styleboxes = 0;
    __draw_clear_glyph_cache(c);
    __draw_invalidate(c, &c->white);
}

uint64_t r_draw_atlas_alloc(r_draw_atlas* a, int w, int h)
{
    if(w <= 0 || h <= 0) return 0;
    if(h + 1 > 1066 || w + 1 > a->w || w > 4095) { a->full = true; return 0; } // Our largest possible buckets holds this height.
    if(a->generation == 0) a->generation = 1;
    int bucket_height = 4; int bucket = 0;
    while(bucket_height < h + 1)
    {
        bucket_height += bucket_height >> 1;
        bucket++;
    }
    if(a->rows[bucket].remaining_w < w + 1)
    {
        if((a->cur_y + bucket_height) > a->h) { a->full = true; return 0; }
        a->rows[bucket].y = a->cur_y;
        a->rows[bucket].remaining_w = a->w;
        a->cur_y += bucket_height;
    }
    uint64_t x = (uint64_t)(a->w - a->rows[bucket].remaining_w), y = (uint64_t)a->rows[bucket].y;
    a->rows[bucket].remaining_w -= w + 1;
    return x | (y << 12) | ((uint64_t)w << 24) | ((uint64_t)h << 36) | ((uint64_t)a->generation << 48);
}

bool r_draw_atlas_is_valid(r_draw_atlas* a, uint64_t handle)
{
    return handle && (uint16_t)(handle >> 48) == a->generation;
}

rect r_draw_atlas_region(uint64_t handle)
{
    return (rect){ (int)(handle & 0xFFF), (int)((handle >> 12) & 0xFFF), (int)((handle >> 24) & 0xFFF), (int)((handle >> 36) & 0xFFF) };
}

void r_draw_atlas_reset(r_draw_atlas* a)
{
    a->cur_y = 0;
    memset(a->rows, 0, sizeof(a->rows));
    a->full = false;
    if(++a->generation == 0) a->generation = 1;
}

r_draw_style_rule* r_draw_get_style_rule(r_draw* c, enum r_style key)
{
    return hash_get(&c->style_rules, key);
}

void r_draw_set_style_rule(r_draw* c, enum r_style key, r_draw_style_rule *rule)
{
    *(r_draw_style_rule*)hash_insert(&c->style_rules, key) = *rule;
}

void r_draw_delete_style_rule(r_draw* c, enum r_style key)
{
    hash_remove(&c->style_rules, key);
}

r_draw_style_rule* r_draw_find_style_rule(r_draw* c, int style)
{
    static const uint32_t strip_states[] = {
        0, R_STATE_HOVER, R_STATE_PRESSED, R_STATE_PRESSED | R_STATE_HOVER,
        R_STATE_FOCUS, R_STATE_HOVER | R_STATE_FOCUS, R_STATE_PRESSED | R_STATE_HOVER | R_STATE_FOCUS, R_STATE_MASK
    };
    static const uint32_t strip_structure[] = {
        0, R_VARIANT_MASK, R_FLAG_MASK, R_VARIANT_MASK | R_FLAG_MASK,
        R_COMPONENT_MASK, R_COMPONENT_MASK | R_VARIANT_MASK, R_COMPONENT_MASK | R_VARIANT_MASK | R_FLAG_MASK,
        R_STYLE_BASE_MASK, R_STYLE_BASE_MASK | R_COMPONENT_MASK, R_STYLE_BASE_MASK | R_COMPONENT_MASK | R_VARIANT_MASK | R_FLAG_MASK
    };
    if(c->theme) __draw_theme_flush(c);
    uint32_t base = (uint32_t)style;
    r_draw_style_rule* rule = NULL;
    for(int i = 0; i < COUNTOF(strip_structure) && !rule; i++)
    {
        uint32_t structure = base & ~strip_structure[i];
        uint32_t prev = 0xFFFFFFFF;
        for(int j = 0; j < COUNTOF(strip_states) && !rule; j++)
        {
            uint32_t key = structure & ~strip_states[j];
            if(key == prev) continue;
            prev = key;
            rule = r_draw_get_style_rule(c, key);
        }
    }
    return rule;
}

static rect rule_inner_rect(r_draw* c, r_draw_style_rule* rule, rect outer)
{
    rect r = r_draw_stylebox_inner_rect(c, rule->bg_stylebox, outer);
    return r_shrink_sides(r, rule->padding_x, rule->padding_y, rule->padding_x, rule->padding_y);
}

static rect __draw_calc_inner_rect(r_context* ctx, int style, rect outer)
{
    r_draw_style_rule* rule = r_draw_find_style_rule(ctx->userptr, style);
    return rule ? rule_inner_rect(ctx->userptr, rule, outer) : outer;
}

static rect __draw_calc_outer_rect(r_context* ctx, int style, rect* inner)
{
    r_draw_style_rule* rule = r_draw_find_style_rule(ctx->userptr, style);
    if(!rule) return *inner;
    rect padded = r_grow_sides(*inner, rule->padding_x, rule->padding_y, rule->padding_x, rule->padding_y);
    rect outer = r_draw_stylebox_outer_rect(ctx->userptr, rule->bg_stylebox, &padded);
    *inner = r_shrink_sides(padded, rule->padding_x, rule->padding_y, rule->padding_x, rule->padding_y);
    return outer;
}

static rect __draw_measure_text(r_context* ctx, int style, const char* text, int len, int wrap_width)
{
    r_draw_style_rule* rule = r_draw_find_style_rule(ctx->userptr, style);
    return r_draw_measure_text_wrapped(ctx->userptr, rule ? rule->label_font : NULL, text, len, wrap_width);
}

static int __draw_break_text(r_context* ctx, int style, const char* text, int len, int width, bool force, int* w)
{
    r_draw_style_rule* rule = r_draw_find_style_rule(ctx->userptr, style);
    return r_draw_break_text(ctx->userptr, rule ? rule->label_font : NULL, text, len, width, force, w);
}

static void layout_defaults(r_draw* c, int style, int* size, int* spacing)
{
    r_draw_style_rule* rule = r_draw_find_style_rule(c, style);
    r_draw_style_rule* fallback = r_draw_get_style_rule(c, R_STYLE_FALLBACK);
    *size    = rule && rule->size    ? rule->size    : fallback ? fallback->size    : 0;
    *spacing = rule && rule->spacing ? rule->spacing : fallback ? fallback->spacing : 0;
}

static void __draw_get_layout(r_context* ctx, int style, int* size, int* spacing)
{
    layout_defaults(ctx->userptr, style, size, spacing);
}

static int icon_metrics(r_draw* c, r_draw_style_rule* rule, int* gap)
{
    int size, spacing;
    layout_defaults(c, R_STYLE_ICON, &size, &spacing);
    *gap = spacing * 3 / 4;
    return rule && rule->icon_size ? rule->icon_size : size;
}

static rect __draw_measure_icon(r_context* ctx, int style, int icon, int* gap)
{
    int s = icon_metrics(ctx->userptr, r_draw_find_style_rule(ctx->userptr, style), gap);
    return (rect){.w = s, .h = s};
}

static int text_reach(const r_draw_style_rule* rule)
{
    float outline = rule->text_outline_color >> 24 ? rule->text_outline : 0, reach = outline;
    if(rule->text_shadow_color >> 24)
        reach = fmaxf(reach, fmaxf(fabsf(rule->text_shadow_x), fabsf(rule->text_shadow_y)) + outline + rule->text_shadow_blur * 2);
    return (int)ceilf(reach);
}

void __draw_emit(r_draw* c, uint64_t handle, uint32_t color, r_frect src, r_frect dst, rect clip)
{
    float x0 = MAX(dst.x, (float)clip.x), y0 = MAX(dst.y, (float)clip.y);
    float x1 = MIN(dst.x + dst.w, (float)(clip.x + clip.w)), y1 = MIN(dst.y + dst.h, (float)(clip.y + clip.h));
    if(!handle || x1 <= x0 || y1 <= y0) return;
    float kx = src.w / dst.w, ky = src.h / dst.h;
    src = (r_frect){ src.x + (x0 - dst.x) * kx, src.y + (y0 - dst.y) * ky, (x1 - x0) * kx, (y1 - y0) * ky };

    if(c->num_quads == c->cap_quads)
    {
        c->cap_quads = c->cap_quads ? c->cap_quads * 2 : 1024;
        c->quads = realloc(c->quads, c->cap_quads * sizeof(r_draw_quad));
    }
    c->quads[c->num_quads++] = (r_draw_quad){ handle, color, src, (r_frect){ x0, y0, x1 - x0, y1 - y0 }, c->pass_transform };
}

void r_draw_render_fill(r_draw* c, uint32_t color, rect bounds)
{
    static const uint32_t white[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
    rect px = __draw_to_pixels(bounds, c->scale);
    __draw_emit(c, __draw_cached(c, &c->white, white, 2, 2), __draw_premult_color(color), (r_frect){ 0, 0, 2, 2 }, (r_frect){ px.x, px.y, px.w, px.h }, c->clip_px);
}

static void list_matrix(r_ui_list* l, int ox, int oy, float lm[2][3])
{
    float (*m)[3] = l->content_transform;
    for(int i = 0; i < 2; i++)
    {
        lm[i][0] = m[i][0]; lm[i][1] = m[i][1];
        lm[i][2] = (i ? oy : ox) - m[i][0] * ox - m[i][1] * oy + m[i][2];
    }
}

static rect map_rect(float m[2][3], rect r)
{
    float x0 = 1e30f, y0 = 1e30f, x1 = -1e30f, y1 = -1e30f;
    for(int i = 0; i < 4; i++)
    {
        float px = r.x + (i & 1 ? r.w : 0), py = r.y + (i & 2 ? r.h : 0);
        float x = m[0][0] * px + m[0][1] * py + m[0][2], y = m[1][0] * px + m[1][1] * py + m[1][2];
        x0 = MIN(x0, x); y0 = MIN(y0, y); x1 = MAX(x1, x); y1 = MAX(y1, y);
    }
    int ix = (int)floorf(x0), iy = (int)floorf(y0);
    return (rect){ ix, iy, (int)ceilf(x1) - ix, (int)ceilf(y1) - iy, r.id };
}

void r_draw_render_pass_begin(r_draw* c, r_ui_list* l)
{
    c->pass_list = l;
    r_list_get_transform(l, &c->pass_ox, &c->pass_oy, &c->pass_clip);
    c->clip_px = __draw_to_pixels(c->pass_clip, c->scale);
    c->pass_transform = NULL;
    if(!l->use_transform_matrix || c->num_transforms == COUNTOF(c->transforms)) return;

    r_draw_transform* t = &c->transforms[c->num_transforms++];
    float lm[2][3], inv[2][3];
    list_matrix(l, c->pass_ox, c->pass_oy, lm);
    for(int i = 0; i < 2; i++)
        t->m[i][0] = lm[i][0], t->m[i][1] = lm[i][1], t->m[i][2] = lm[i][2] * c->scale;
    t->clip = c->clip_px;
    c->pass_clip = __draw_invert(lm, inv) ? r_grow(map_rect(inv, c->pass_clip), 1) : (rect){0};
    c->clip_px = __draw_to_pixels(c->pass_clip, c->scale);
    c->pass_transform = t;
}

void r_draw_render_pass_end(r_draw* c)
{
    c->pass_list = NULL;
    c->clip_px = no_clip;
    c->pass_transform = NULL;
}

static void render_content(r_draw* c, r_draw_style_rule* rule, r_drawable* d, rect area, rect bounds)
{
    uint32_t icon_color = rule->icon_color ? rule->icon_color : rule->label_color;
    r_alignment align = rule->label_align ? rule->label_align : R_LEFT_CENTER;
    int gap, icon_size = icon_metrics(c, rule, &gap);
    rect clip = rect_intersect(bounds, c->pass_clip);
    area = r_offset(area, d->label_offset_x, d->label_offset_y);
    if(d->wrap_label) align = (align & ~(R_V_CENTER | R_BOTTOM)) | R_TOP;
    int wrap = d->wrap_label ? area.w : 0;

    if(d->icon)
    {
        rect ir = {.w = icon_size, .h = icon_size};
        if(d->text)
        {
            if(wrap) wrap = MAX(wrap - icon_size - gap, 1);
            rect t = r_draw_measure_text_wrapped(c, rule->label_font, d->text, -1, wrap);
            rect group = {.w = icon_size + gap + t.w, .h = t.h};
            r_align(align, area, &group);
            rect line = {.x = group.x, .y = group.y, .w = icon_size, .h = r_draw_measure_text(c, rule->label_font, "", -1).h};
            r_align(R_LEFT_CENTER, line, &ir);
            area = r_shrink_sides((rect){group.x, area.y, group.w, area.h}, icon_size + gap, 0, 0, 0);
            align = (align & (R_TOP | R_BOTTOM | R_V_CENTER)) | R_LEFT;
        }
        else r_align(align, area, &ir);
        if(rect_intersect(ir, clip).w > 0) r_draw_render_icon(c, (uint16_t)d->icon, icon_color, ir);
    }
    if(d->text) __draw_render_text(c, d, rule, area, align, clip, wrap);
}

static int interaction_state(r_draw* c, r_drawable* d)
{
    r_context* ctx = c->context;
    uint64_t id = d->parent_id ? d->parent_id : d->r.id;
    if(!id) return 0;
    int state = 0;
    if(ctx->hover_id == id && !(ctx->pressed_id && ctx->pressed_id != id)) state |= R_STATE_HOVER;
    if(r_is_pressed_visual(ctx, id)) state |= R_STATE_PRESSED;
    if(ctx->focus_id == id && (ctx->is_in_navigation_mode || r_focus_wants_text(ctx))) state |= R_STATE_FOCUS;
    return state;
}

void r_draw_render_drawable(r_draw* c, r_drawable* item)
{
    r_drawable e = r_drawable_eval(c->context, item), *d = &e;
    c->pass_item = item;
    if(d->r.w <= 0 || d->r.h <= 0) return;
    rect r = r_offset(d->r, c->pass_ox, c->pass_oy);
    rect reach = r_grow(r, 32);
    if(rect_intersect(reach, c->pass_clip).w <= 0) return;
    if(!c->pass_transform)
    {
        rect in = rect_intersect(reach, c->area);
        if(in.w <= 0 || in.h <= 0) return;
    }

    int base_style = d->style & R_STYLE_BASE_MASK;
    if(base_style == R_STYLE_IMAGE) { render_image(c, d->data, r); return; }
    bool content_only = base_style == R_STYLE_TEXT || base_style == R_STYLE_ICON;
    int style = content_only && d->outer_style ? (d->outer_style & ~R_FLAG_EMBEDDED) | (d->style & R_STATE_MASK) : d->style;
    int state = interaction_state(c, d) | (style & R_STATE_MASK);
    r_draw_style_rule* rule = r_draw_find_style_rule(c, style | state);

    if(!rule)
    {
        r_draw_render_fill(c, r_rgb_u8(255, 0, 0), r);
        if(d->text) r_draw_render_text(c, 0, d->text, r_rgb_u8(0, 255, 0), r_grow_sides(r, -4, -4, -4, -4));
        return;
    }

    if(!content_only)
    {
        if((d->style & R_COMPONENT_MASK) == R_COMPONENT_CARET && fmodf(c->context->time_since_focus, 1.0f) >= 0.5f) return;
        r_draw_render_stylebox(c, rule->bg_stylebox, r);
    }
    if(d->text || d->icon)
        render_content(c, rule, d, content_only ? r : rule_inner_rect(c, rule, r), content_only ? r_grow(r, 1 + text_reach(rule)) : r);
    if(!content_only)
    {
        r_draw_render_stylebox(c, rule->overlay, r);
        if(state & R_STATE_HOVER) r_draw_render_stylebox(c, rule->hover_overlay, r);
        if(state & R_STATE_FOCUS) r_draw_render_stylebox(c, rule->focus_overlay, r);
    }
}

void r_draw_render_area(r_draw* c, uint64_t target, rect area)
{
    if(c->glyphs_full) __draw_clear_glyph_cache(c);
    c->cache_full = false;
    c->num_quads = 0;
    c->num_transforms = 0;
    int x0 = (int)floorf(area.x / c->scale) - 1, y0 = (int)floorf(area.y / c->scale) - 1;
    int x1 = (int)ceilf((area.x + area.w) / c->scale) + 1, y1 = (int)ceilf((area.y + area.h) / c->scale) + 1;
    c->area = (rect){ x0, y0, x1 - x0, y1 - y0 };
    for(r_ui_list* l = c->context->chain_start; l != NULL; l = l->chain_next)
    {
        if(!r_list_is_visible(l) || l->target != target) continue;
        r_draw_render_pass_begin(c, l);
        for(int i = 0; i < l->count_draw_items; i++)
            r_draw_render_drawable(c, &l->draw_items[i]);
        r_draw_render_pass_end(c);
    }
    c->area = no_clip;
    c->backend.submit(c->backend.userptr, c->quads, c->num_quads);
    if(c->cache_full || c->glyphs_full)
    for(r_ui_list* l = c->context->chain_start; l != NULL; l = l->chain_next)
        if(l->target == target) l->needs_redraw = true;
}

void r_draw_render(r_draw* c, uint64_t target)
{
    r_draw_render_area(c, target, no_clip);
}

static rect union_area(rect a, rect b)
{
    if(b.w <= 0 || b.h <= 0) return a;
    return a.w > 0 && a.h > 0 ? rect_expand(a, b) : b;
}

typedef struct { int ox, oy; rect clip; float lm[2][3]; } list_placement;

static list_placement place_list(r_ui_list* l)
{
    list_placement p;
    r_list_get_transform(l, &p.ox, &p.oy, &p.clip);
    list_matrix(l, p.ox, p.oy, p.lm);
    return p;
}

static rect drawable_bounds(r_draw* c, r_ui_list* l, list_placement* p, r_drawable* d)
{
    if(d->r.w <= 0 || d->r.h <= 0) return (rect){0};
    rect r = r_offset(d->r, p->ox, p->oy);
    int base_style = d->style & R_STYLE_BASE_MASK;
    if(base_style != R_STYLE_TEXT && base_style != R_STYLE_ICON)
    {
        static const int states[] = { 0, R_STATE_HOVER, R_STATE_PRESSED | R_STATE_HOVER, R_STATE_FOCUS, R_STATE_FOCUS | R_STATE_HOVER, R_STATE_FOCUS | R_STATE_PRESSED | R_STATE_HOVER };
        int e[4] = {0};
        r_draw_style_rule* prev = NULL;
        for(int si = 0; si < COUNTOF(states); si++)
        {
            r_draw_style_rule* rule = r_draw_find_style_rule(c, d->style | states[si]);
            if(!rule || rule == prev) continue;
            prev = rule;
            int slots[4] = { rule->bg_stylebox, rule->overlay, rule->hover_overlay, rule->focus_overlay };
            for(int k = 0; k < 4; k++)
            {
                if(slots[k] <= 0 || slots[k] >= RCUT_DRAW_MAX_STYLEBOXES) continue;
                struct r_draw_stylebox* sb = &c->styleboxes[slots[k]];
                float s = sb->source_scale > 0.0f ? sb->source_scale : 1.0f;
                for(int j = 0; j < 4; j++) e[j] = MAX(e[j], (int)ceilf(sb->side_drawn[j] / s));
            }
        }
        r = r_grow_sides(r, e[0], e[1], e[2], e[3]);
    }
    else
    {
        r_draw_style_rule* rule = r_draw_find_style_rule(c, d->outer_style ? (d->outer_style & ~R_FLAG_EMBEDDED) | (d->style & R_STATE_MASK) : d->style);
        if(rule) r = r_grow(r, 1 + text_reach(rule));
    }
    if(l->use_transform_matrix) r = map_rect(p->lm, r);
    return rect_intersect(r, p->clip);
}

static rect list_bounds(r_draw* c, r_ui_list* l)
{
    rect bounds = {0};
    list_placement p = place_list(l);
    for(int i = 0; i < l->count_draw_items; i++)
    {
        bounds = union_area(bounds, drawable_bounds(c, l, &p, &l->draw_items[i]));
    }
    return bounds;
}

static rect take_data_damage(r_draw* c, r_ui_list* l)
{
    rect damage = {0}, r;
    list_placement p = place_list(l);
    for(int i = 0; i < l->count_draw_items; i++)
    {
        r_drawable d = l->draw_items[i];
        if(!r_drawable_take_change(&l->draw_items[i], &r)) continue;
        d.r = r;
        damage = union_area(damage, drawable_bounds(c, l, &p, &d));
    }
    return damage;
}

rect r_draw_target_bounds(r_draw* c, uint64_t target)
{
    rect bounds = {0};
    for(r_ui_list* l = c->context->chain_start; l; l = l->chain_next)
        if(l->target == target && r_list_is_visible(l)) bounds = union_area(bounds, list_bounds(c, l));
    return bounds;
}

static rect id_damage(r_ui_list* l)
{
    if(l->damage.w <= 0 || l->damage.h <= 0) return (rect){0};
    list_placement p = place_list(l);
    rect r = r_offset(l->damage, p.ox, p.oy);
    if(l->use_transform_matrix) r = map_rect(p.lm, r);
    return rect_intersect(r, p.clip);
}

static int theme_reach(r_draw* c)
{
    int reach = 0;
    for(int i = 1; i < RCUT_DRAW_MAX_STYLEBOXES; i++)
    {
        struct r_draw_stylebox* sb = &c->styleboxes[i];
        float s = sb->source_scale > 0.0f ? sb->source_scale : 1.0f;
        for(int j = 0; j < 4; j++) reach = MAX(reach, (int)ceilf(sb->side_drawn[j] / s));
    }
    for(int i = 0; i < c->style_rules.num_slots; i++)
        if(c->style_rules.keys[i] >= 0) reach = MAX(reach, 1 + text_reach(hash_value_at(&c->style_rules, i)));
    return reach;
}

static rect take_list_damage(r_draw* c, r_ui_list* l, uint64_t target)
{
    rect damage = {0};
    bool mine  = l->context && l->target == target;
    bool shown = mine && r_list_is_visible(l);
    if(l->drawn_target == target && l->drawn_area.w > 0 && (!shown || l->needs_redraw))
    {
        damage = l->drawn_area;
        l->drawn_area = (rect){0};
    }
    if(mine && !shown) l->needs_redraw = true;
    if(shown && l->needs_redraw)
    {
        int ox, oy;
        r_list_get_transform(l, &ox, &oy, &l->drawn_area);
        if(l->ephemeral_parent && !l->is_viewport && !l->is_masked) l->drawn_area = list_bounds(c, l);
        l->drawn_target = target;
        l->needs_redraw = false;
        damage = union_area(damage, l->drawn_area);
    }
    return damage;
}

rect r_draw_take_damage(r_draw* c, uint64_t target)
{
    r_context* ctx = c->context;

    uint64_t hover, pressed, focus;
    hover   = ctx->pressed_id && ctx->pressed_id != ctx->hover_id ? 0 : ctx->hover_id;
    pressed = r_is_pressed_visual(ctx, ctx->pressed_id) ? ctx->pressed_id : 0;
    focus   = ctx->is_in_navigation_mode || r_focus_wants_text(ctx) ? ctx->focus_id : 0;
    if(hover   != ctx->drawn_hover_id)   { r_damage_id(ctx, ctx->drawn_hover_id);   r_damage_id(ctx, hover); }
    if(pressed != ctx->drawn_pressed_id) { r_damage_id(ctx, ctx->drawn_pressed_id); r_damage_id(ctx, pressed); }
    if(focus   != ctx->drawn_focus_id)   { r_damage_id(ctx, ctx->drawn_focus_id);   r_damage_id(ctx, focus); }
    ctx->drawn_hover_id = hover; ctx->drawn_pressed_id = pressed; ctx->drawn_focus_id = focus;

    bool caret = fmodf(ctx->time_since_focus, 1.0f) < 0.5f;
    if(caret != c->drawn_caret && r_focus_wants_text(ctx)) r_damage_id(ctx, ctx->focus_id);
    c->drawn_caret = caret;

    rect damage = {0};
    for(r_ui_list* l = ctx->chain_start; l; l = l->chain_next)
        if(l->target == target && r_list_is_visible(l)) damage = union_area(damage, take_data_damage(c, l));
    for(int i = 0; i < ctx->count_ui_lists; i++)  damage = union_area(damage, take_list_damage(c, &ctx->ui_lists[i], target));
    for(int i = 0; i < ctx->count_ephemeral; i++) damage = union_area(damage, take_list_damage(c, &ctx->ephemeral[i], target));
    for(r_ui_list* l = ctx->chain_start; l; l = l->chain_next)
        if(l->target == target && r_list_is_visible(l)) damage = union_area(damage, id_damage(l));
    if(damage.w <= 0 || damage.h <= 0) return (rect){0};
    return __draw_to_pixels(r_grow(damage, 1 + theme_reach(c)), c->scale);
}
