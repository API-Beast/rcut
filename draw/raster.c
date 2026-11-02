#include "internal.h"

void r_stylebox_set_outline_border(r_draw_stylebox_def*  box, uint32_t color, float border_thickness, float border_radius)
{
    for(int i = 0; i<4; i++)
    {
        box->border_blur[i] = 0.0f;
        box->border_gradient[i].base_color = color;
        box->border_gradient[i].use_gradient = false;
        box->border_thickness[i] = border_thickness;
        box->corner_radius[i] = border_radius;
    }
}

void r_stylebox_set_shadow(r_draw_stylebox_def* box, int slot, float offset_x, float offset_y, float blur, float opacity, bool inset)
{
    assert(slot < 4 && slot >= 0);
    box->shadows[slot].blur = blur;
    box->shadows[slot].color = r_rgba_f(0.0f, 0.0f, 0.0f, opacity);
    box->shadows[slot].is_inset = inset;
    box->shadows[slot].offset_x = offset_x;
    box->shadows[slot].offset_y = offset_y;
}

void r_stylebox_set_bg_color(r_draw_stylebox_def* box, uint32_t color)
{
    box->bg_gradients[0].base_color = color;
    box->bg_gradients[0].use_gradient = false;
}

void r_stylebox_set_bg_vertical_gradient(r_draw_stylebox_def* box, uint32_t* stops, int num_stops)
{
    assert(num_stops <= 8 && num_stops >= 2);
    box->bg_gradients[1].base_color = 0x0;
    box->bg_gradients[1].use_gradient = true;
    box->bg_gradients[1].is_elliptical = false;
    box->bg_gradients[1].num_stops = num_stops;
    box->bg_gradients[1].origin_x = 0.5f;
    box->bg_gradients[1].origin_y = 0.0f;
    box->bg_gradients[1].end_x = 0.5f;
    box->bg_gradients[1].end_y = 1.0f;
    for(int i = 0; i < num_stops; i++)
    {
        box->bg_gradients[1].stops[i].position = (float)i / (num_stops - 1);
        box->bg_gradients[1].stops[i].color    = stops[i];
    }
}

static bool render_stylebox_exact(r_draw* c, int slot, r_frect dst, uint32_t color);
static uint64_t stylebox_handle(r_draw* c, struct r_draw_stylebox* sb, bool bg);

static void emit_9grid(r_draw* c, uint64_t handle, uint32_t color, int w, int h, const int sides[4], float scale, r_frect dst)
{
    float sx[4] = { 0, sides[0], w - sides[2], w }, sy[4] = { 0, sides[1], h - sides[3], h };
    float dx[4] = { dst.x, dst.x + ceilf(sides[0] * scale), dst.x + dst.w - ceilf(sides[2] * scale), dst.x + dst.w };
    float dy[4] = { dst.y, dst.y + ceilf(sides[1] * scale), dst.y + dst.h - ceilf(sides[3] * scale), dst.y + dst.h };
    for(int j = 0; j < 3; j++)
    for(int i = 0; i < 3; i++)
        r_draw__emit(c, handle, color, (r_frect){ sx[i], sy[j], sx[i+1] - sx[i], sy[j+1] - sy[j] }, (r_frect){ dx[i], dy[j], dx[i+1] - dx[i], dy[j+1] - dy[j] }, c->clip_px);
}

void r_draw_render_stylebox(r_draw* c, int stylebox, rect outer)
{
    if(stylebox <= 0 || stylebox >= RCUT_DRAW_MAX_STYLEBOXES) return;
    struct r_draw_stylebox* sb = &c->styleboxes[stylebox];
    if(sb->w <= 0) return;
    float s = sb->source_scale > 0.0f ? sb->source_scale : 1.0f;
    float ps = c->scale;
    float x0 = roundf(roundf(outer.x * ps) - sb->side_expand[0] * ps / s), y0 = roundf(roundf(outer.y * ps) - sb->side_expand[1] * ps / s);
    float x1 = roundf(roundf((outer.x + outer.w) * ps) + sb->side_expand[2] * ps / s), y1 = roundf(roundf((outer.y + outer.h) * ps) + sb->side_expand[3] * ps / s);
    r_frect dst = { x0, y0, x1 - x0, y1 - y0 };

    float corner = MIN(dst.w / ps * s / (float)(sb->side_width[0] + sb->side_width[2]), dst.h / ps * s / (float)(sb->side_width[1] + sb->side_width[3]));
    if(corner > 1.0f) corner = 1.0f;

    uint32_t color = 0xFFFFFFFF;
    if(corner < 1.0f && !sb->has_separate_background && !sb->permanent && render_stylebox_exact(c, stylebox, dst, color)) return;
    if(sb->has_separate_background)
    {
        r_frect bg = { dst.x + sb->side_bg_inset[0] * ps / s, dst.y + sb->side_bg_inset[1] * ps / s,
                       dst.w - (sb->side_bg_inset[0] + sb->side_bg_inset[2]) * ps / s, dst.h - (sb->side_bg_inset[1] + sb->side_bg_inset[3]) * ps / s };
        float fit = MAX(bg.w / sb->bg_w, bg.h / sb->bg_h);
        r_frect src = { 0, 0, bg.w / fit, bg.h / fit };
        src.x = (sb->bg_w - src.w) / 2;
        src.y = (sb->bg_h - src.h) / 2;
        r_draw__emit(c, stylebox_handle(c, sb, true), color, src, bg, c->clip_px);
    }
    emit_9grid(c, stylebox_handle(c, sb, false), color, sb->w, sb->h, sb->side_width, corner * ps / s, dst);
}

rect r_draw_stylebox_inner_rect(r_draw* c, int slot, rect outer)
{
    if(slot < 0 || slot >= (int)(sizeof(c->styleboxes) / sizeof(c->styleboxes[0]))) return outer;
    struct r_draw_stylebox* sb = &c->styleboxes[slot];
    return r_shrink_sides(outer, sb->side_inset[0], sb->side_inset[1], sb->side_inset[2], sb->side_inset[3]);
}

rect r_draw_stylebox_outer_rect(r_draw* c, int slot, rect* inner)
{
    if(slot < 0 || slot >= (int)(sizeof(c->styleboxes) / sizeof(c->styleboxes[0]))) return *inner;
    struct r_draw_stylebox* sb = &c->styleboxes[slot];
    r_min_size(inner, sb->min_w - sb->side_inset[0] - sb->side_inset[2], sb->min_h - sb->side_inset[1] - sb->side_inset[3]);
    return r_grow_sides(*inner, sb->side_inset[0], sb->side_inset[1], sb->side_inset[2], sb->side_inset[3]);
}


static inline float len(float x, float y){ return sqrtf(x*x+y*y);}

static inline int max(int a, int b){ return a > b ? a : b; }
static inline float maxf(float a, float b){ return a > b ? a : b; }

typedef struct
{
    int min_x, min_y, max_x, max_y, w, h, inner_w, inner_h, ext_w, ext_h;
    int drawn_w, drawn_h;
    int pad_w, pad_h;
} sb_cell;

static inline sb_cell calc_cel_size(r_draw_stylebox_def* d, int x, int y)
{
    bool is_corner = (x != 1) && (y != 1);
    sb_cell r = {.ext_w = 2, .ext_h = 2};
    int h_edge = (x == 0) ? 0 : 2;
    int v_edge = (y == 0) ? 1 : 3;
    // Edge
    if(x != 1) {
        
        r.w = d->border_thickness[h_edge];
        r.inner_w = r.pad_w = d->border_blur[h_edge];
    }
    if(y != 1) {
        
        r.h = d->border_thickness[v_edge];
        r.inner_h = r.pad_h = d->border_blur[v_edge];
    }
    // Corner
    if(is_corner)
    {
        int idx = (x == 2) + 2 * (y == 2);
        r.w = max(r.w, d->corner_radius[idx]);
        r.h = max(r.h, d->corner_radius[idx]);
        for(int i = 0; i < 4; i++)
        {
            if(!d->shadows[i].color) continue;
            float reach = d->corner_radius[idx] + d->shadows[i].blur / 2;
            r.w = max(r.w, (int)ceilf((x == 0 ? 1 : -1) * d->shadows[i].offset_x + reach));
            r.h = max(r.h, (int)ceilf((y == 0 ? 1 : -1) * d->shadows[i].offset_y + reach));
        }
    }
    // Center
    if(x == 1 && y == 1)
    {
        if(d->bg_preserve_aspect_ratio)
        {
            r.w = 4;
            r.h = 4;
        }
        else
        {
            r.w = max(d->bg_raster_width, 4);
            r.h = max(d->bg_raster_height, 4);
        }
    }
    else
    {
        int ext_w_dir = x < 1 ? -1 : 1;
        int ext_h_dir = y < 1 ? -1 : 1;
        for(int i = 0; i < 4; i++)
        {
            if(d->shadows[i].is_inset)
            {
                r.inner_w = max(r.inner_w, ext_w_dir * d->shadows[i].offset_x + d->shadows[i].blur + d->shadows[i].grow);
                r.inner_h = max(r.inner_h, ext_h_dir * d->shadows[i].offset_y + d->shadows[i].blur + d->shadows[i].grow);
            }
            else
            {
                r.drawn_w = max(r.drawn_w, ext_w_dir * d->shadows[i].offset_x + d->shadows[i].blur + d->shadows[i].grow);
                r.drawn_h = max(r.drawn_h, ext_h_dir * d->shadows[i].offset_y + d->shadows[i].blur + d->shadows[i].grow);
                r.ext_w = max(r.ext_w, r.drawn_w);
                r.ext_h = max(r.ext_h, r.drawn_h);
            }
        }
    }
    return r;
}

static inline float sdf_rounded_box(float x, float y, float w, float h, float a, float b, float c, float d)
{
    float r = x < 0 ? (y < 0 ? a : c) : (y < 0 ? b : d);
    float rel_x = fabs(x) - w/2 + r;
    float rel_y = fabs(y) - h/2 + r;
    if(rel_x > 0 && rel_y > 0)
        return len(rel_x, rel_y) - r;
    if(rel_x > 0 || rel_x > rel_y)
        return rel_x - r;
    return rel_y - r;
}

static inline void l1_normal_rounded_box(float x, float y, float w, float h, float a, float b, float c, float d, float* dir_x, float* dir_y)
{
    float r = x < 0 ? (y < 0 ? a : c) : (y < 0 ? b : d);
    float rel_x = fabs(x) - w/2 + r;
    float rel_y = fabs(y) - h/2 + r;
    if(rel_x > 0 && rel_y > 0)
    {
        *dir_x = copysignf(1.0f, x) * rel_x / (fabs(rel_x)+fabs(rel_y));
        *dir_y = copysignf(1.0f, y) * rel_y / (fabs(rel_x)+fabs(rel_y));
        return;
    }
    if(rel_x > 0 || rel_x > rel_y)
    {
        *dir_x = copysignf(1.0f, x);
        *dir_y = 0.0f;
        return;
    }
    *dir_x = 0.0f;
    *dir_y = copysignf(1.0f, y);
    return;
}

static inline uint32_t mask_alpha(uint32_t col, float mask)
{
    uint8_t bytes[4]; memcpy(bytes, &col, 4);
    bytes[3] *= mask;
    memcpy(&col, bytes, 4);
    return col;
}

static inline float clamp_unorm(float v){ return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }
static inline uint8_t clamp_u8(uint8_t v){ return v < 0 ? 0 : v > 255 ? 255 : v; }

static inline uint32_t alpha_over(uint32_t dst, uint32_t src, float mask)
{
    uint8_t dst_bytes[4]; memcpy(dst_bytes, &dst, 4);
    uint8_t src_bytes[4]; memcpy(src_bytes, &src, 4);
    src_bytes[3] = src_bytes[3] * mask;
    int16_t new_alpha = src_bytes[3] + dst_bytes[3] * (255 - src_bytes[3]) / 255;
    if(new_alpha != 0)
    for(int i = 0; i < 3; i++)
        dst_bytes[i] = (src_bytes[i] * src_bytes[3] + 
                        dst_bytes[i] * dst_bytes[3] * (255 - src_bytes[3]) / 255) / 
                        new_alpha;
    dst_bytes[3] = new_alpha;
    memcpy(&dst, dst_bytes, 4);
    return dst;
}

static inline float linearize(float val)
{
	if(val > 0.04045)
		return pow((val + 0.055) / 1.055, 2.4);
	else
		return val / 12.92;
}

static inline float delinearize(float val)
{
	if(val > 0.0031308)
		return (1.055 * pow(val, 1.0/2.4)) - 0.055;
	else
		return 12.92 * val;
}

static float   linear_table[256];
static uint8_t srgb_table[4096];

static void init_srgb_tables(void)
{
    if(linear_table[255] != 0.0f) return;
    for(int i = 0; i < 256; i++)  linear_table[i] = linearize(i / 255.0f);
    for(int i = 0; i < 4096; i++) srgb_table[i] = (uint8_t)(clamp_unorm(delinearize(i / 4095.0f)) * 255.0f + 0.5f);
}

static inline float   linearize_u8(uint8_t v)   { return linear_table[v]; }
static inline uint8_t delinearize_u8(float v)   { return srgb_table[(int)(clamp_unorm(v) * 4095.0f + 0.5f)]; }

static inline uint32_t alpha_over_linear(uint32_t dst, uint32_t src, float mask)
{
    uint8_t dst_bytes[4]; memcpy(dst_bytes, &dst, 4);
    uint8_t src_bytes[4]; memcpy(src_bytes, &src, 4);
    src_bytes[3] = src_bytes[3] * mask;
    if(src_bytes[3] == 0)   return dst;
    if(src_bytes[3] == 255 || dst_bytes[3] == 0) { memcpy(&src, src_bytes, 4); return src; }
    float dst_linear[3], src_linear[3];
    for(int i = 0; i < 3; i++) {
        dst_linear[i] = linearize_u8(dst_bytes[i]);
        src_linear[i] = linearize_u8(src_bytes[i]);
    }

    float src_alpha = src_bytes[3] / 255.0f;
    float dst_alpha = dst_bytes[3] / 255.0f;
    float new_alpha = src_alpha + dst_alpha * (1.0f - src_alpha);
    float result_linear[3];

    if(new_alpha != 0.0f)
    for(int i = 0; i < 3; i++)
        result_linear[i] = (src_linear[i] * src_alpha + 
                            dst_linear[i] * dst_alpha * (1.0f - src_alpha)) / 
                            new_alpha;

    uint8_t result_bytes[4];
    for(int i = 0; i < 3; i++)
        result_bytes[i] = delinearize_u8(result_linear[i]);
    result_bytes[3] = (uint8_t)(clamp_unorm(new_alpha) * 255.0f + 0.5f);
    
    memcpy(&dst, result_bytes, 4);
    return dst;
}

static inline uint32_t premult(uint32_t value)
{
    uint8_t dst_bytes[4]; memcpy(dst_bytes, &value, 4);
    for(int i = 0; i < 3; i++)
        dst_bytes[i] = dst_bytes[i] * dst_bytes[3] / 255;
    uint32_t dst; memcpy(&dst, dst_bytes, 4);
    return dst;
}

static inline uint32_t finish_pixel(uint32_t value, const r_draw_stylebox_def* d)
{
    return premult(d->transparency > 0.0f ? mask_alpha(value, 1.0f - d->transparency) : value);
}

static inline uint32_t lerp_col(uint32_t dst, uint32_t src, float t)
{
    uint8_t dst_bytes[4]; memcpy(dst_bytes, &dst, 4);
    uint8_t src_bytes[4]; memcpy(src_bytes, &src, 4);
    for(int i = 0; i < 4; i++) dst_bytes[i] = dst_bytes[i] * (1.0f - t) + src_bytes[i] * t;
    memcpy(&dst, dst_bytes, 4);
    return dst;
}

static inline float lerp(float dst, float src, float t)
{
    return dst * (1.0f - t) + src * t;
}

static inline float gamma_lerp(float a, float b, float f, float gamma)
{
    if(a < b) return lerp(a, b, powf(f, gamma));
    else       return lerp(a, b, 1.0f - powf(1.0f - f, gamma));
}

static inline uint32_t lerp_col_gamma(uint32_t dst, uint32_t src, float t)
{
    uint8_t dst_bytes[4]; memcpy(dst_bytes, &dst, 4);
    uint8_t src_bytes[4]; memcpy(src_bytes, &src, 4);
    float result_linear[3];
    for(int i = 0; i < 3; i++)
        result_linear[i] = gamma_lerp(linearize_u8(dst_bytes[i]), linearize_u8(src_bytes[i]), t, 1.6f);

    uint8_t result_bytes[4];
    for(int i = 0; i < 3; i++)
        result_bytes[i] = delinearize_u8(result_linear[i]);
    result_bytes[3] = dst_bytes[3] * (1.0f - t) + src_bytes[3] * t;
    memcpy(&dst, result_bytes, 4);
    return dst;
}


static inline uint32_t eval_gradient(r_draw_gradient* g, float x, float y, float w, float h)
{
    if(!g->use_gradient) return g->base_color;

    float ox = g->origin_x * w, oy = g->origin_y * h;
    float ax = (g->end_x - g->origin_x) * w, ay = (g->end_y - g->origin_y) * h;
    float t;
    if(g->is_elliptical)
    {
        float radius = len(ax, ay);
        float aspect = g->aspect_ratio != 0.0f ? g->aspect_ratio : 1.0f;
        t = radius > 0.0f ? len(x - ox, (y - oy) / aspect) / radius : 0.0f;
    }
    else
    {
        float len2 = ax*ax + ay*ay;
        t = len2 > 0.0f ? ((x - ox) * ax + (y - oy) * ay) / len2 : 0.0f;
    }
    t = clamp_unorm(t);

    uint32_t prev_c = g->base_color;
    float prev_p = 0.0f;
    for(unsigned int i = 0; i < g->num_stops; i++)
    {
        float p = g->stops[i].position;
        if(t <= p)
            return lerp_col_gamma(prev_c, g->stops[i].color, p > prev_p ? (t - prev_p) / (p - prev_p) : 0.0f);
        prev_c = g->stops[i].color;
        prev_p = p;
    }
    return prev_c;
}

static inline uint32_t background_pixel_at(float x, float y, float w, float h, r_draw_stylebox_def* restrict d)
{
    uint32_t result = 0x00000000;
    for(int i = 0; i < 4; i++)
        result = alpha_over_linear(result, eval_gradient(&d->bg_gradients[i], x, y, w, h), 1.0f);
    return result;
}

static inline uint32_t stylebox_pixel_at(uint32_t bg, float x, float y, float w, float h, r_draw_stylebox_def* restrict d)
{
    float rel_x = x - w/2 + 0.5, rel_y = y - h/2 + 0.5;
    float boxd = sdf_rounded_box(rel_x, rel_y, w, h, d->corner_radius[0], d->corner_radius[1], d->corner_radius[2], d->corner_radius[3]);
    float box_alpha = clamp_unorm(0.5f - boxd);

    int   hor_edge = rel_x < 0 ? 0 : 2;
    int   vert_edge = rel_y < 0 ? 1 : 3;
    float norm_x, norm_y;
    float samp_x, samp_y;
    l1_normal_rounded_box(rel_x-0.25, rel_y-0.25, w, h, d->corner_radius[0], d->corner_radius[1], d->corner_radius[2], d->corner_radius[3], &norm_x, &norm_y);
    l1_normal_rounded_box(rel_x+0.25, rel_y+0.25, w, h, d->corner_radius[0], d->corner_radius[1], d->corner_radius[2], d->corner_radius[3], &samp_x, &samp_y);
    norm_x = (norm_x + samp_x) / 2.0f;
    norm_y = (norm_y + samp_y) / 2.0f;

    uint32_t result = bg;
    float t = fabs(norm_x);
    float border_thickness = lerp(d->border_thickness[vert_edge], d->border_thickness[hor_edge], t);
    float border_blur      = lerp(d->border_blur[vert_edge], d->border_blur[hor_edge], t);
    if((border_thickness + border_blur) > 0.0f)
    {
        float blur_w = 1.0f + border_blur / 2.0f;
        float border_alpha = clamp_unorm(0.5f + (border_thickness + border_blur/2.0f + boxd) / blur_w);
        uint32_t border_color = lerp_col_gamma(eval_gradient(&d->border_gradient[vert_edge], x, y, w, h), eval_gradient(&d->border_gradient[hor_edge], x, y, w, h), t);
        result = alpha_over_linear(result, border_color, border_alpha);
    }
    // Inset Shadows
    for(int i = 0; i < 4; i++)
    {
        if(!d->shadows[i].is_inset) continue;
        if(box_alpha <= 0.0f)       continue;
        float rgrowth = d->shadows[i].blur/2;
        float shadowd = sdf_rounded_box(x - w/2 - d->shadows[i].offset_x, y - h/2 - d->shadows[i].offset_y, w, h,
            d->corner_radius[0] + rgrowth, d->corner_radius[1] + rgrowth, d->corner_radius[2] + rgrowth, d->corner_radius[3] + rgrowth);
        shadowd -= d->shadows[i].grow;
        shadowd = -shadowd;

        float blur_w = d->shadows[i].blur > 1.0f ? d->shadows[i].blur : 1.0f;
        float shadow_alpha = clamp_unorm(0.5f - shadowd / blur_w);
        if(d->shadows[i].hardness > 0.0f) shadow_alpha = powf(shadow_alpha, d->shadows[i].hardness);
        result = alpha_over_linear(result, d->shadows[i].color, shadow_alpha);
    }

    // Mask
    result = mask_alpha(result, box_alpha);

    // Outset Shadows
    for(int i = 0; i < 4; i++)
    {
        if(d->shadows[i].is_inset) continue;
        if(box_alpha >= 1.0f)      continue;
        float rgrowth = d->shadows[i].blur/2;
        float shadowd = sdf_rounded_box(x - w/2 - d->shadows[i].offset_x, y - h/2 - d->shadows[i].offset_y, w, h,
            d->corner_radius[0] + rgrowth, d->corner_radius[1] + rgrowth, d->corner_radius[2] + rgrowth, d->corner_radius[3] + rgrowth);
        shadowd -= d->shadows[i].grow;

        float blur_w = d->shadows[i].blur > 1.0f ? d->shadows[i].blur : 1.0f;
        float shadow_alpha = clamp_unorm(0.5f - shadowd / blur_w);
        if(d->shadows[i].hardness > 0.0f) shadow_alpha = powf(shadow_alpha, d->shadows[i].hardness);

        uint32_t shadow_clr = mask_alpha(d->shadows[i].color, shadow_alpha);
        result = alpha_over_linear(shadow_clr, result, 1.0f);
    }

    return result;
}

static r_draw_stylebox_def scale_def(const r_draw_stylebox_def* d, float s)
{
    r_draw_stylebox_def o = *d;
    for(int i = 0; i < 4; i++)
    {
        o.extra_padding[i] *= s; o.border_thickness[i] *= s; o.border_blur[i] *= s; o.corner_radius[i] *= s;
        o.shadows[i].offset_x *= s; o.shadows[i].offset_y *= s; o.shadows[i].grow *= s; o.shadows[i].blur *= s;
    }
    o.bg_raster_width  = (int)(o.bg_raster_width * s);
    o.bg_raster_height = (int)(o.bg_raster_height * s);
    return o;
}

bool r_draw_generate_stylebox(r_draw* c, int slot, r_draw_stylebox_def* logical_def)
{
    init_srgb_tables();
    float scale = c->scale > 0.0f ? c->scale : 1.0f;
    r_draw_stylebox_def scaled = scale_def(logical_def, scale);
    r_draw_stylebox_def* d = &scaled;
    if(slot <= 0 || slot >= RCUT_DRAW_MAX_STYLEBOXES) return false;

    sb_cell sizes[9];
    for(int i = 0; i < 9; i++)
        sizes[i] = calc_cel_size(d, i%3, i/3);

    int needed_w = 0, needed_h = 0;

    int cur_x = 0;
    for(int x = 0; x < 3; x++)
    {
        for(int y = 0; y < 3; y++)
            sizes[y*3+x].min_x = cur_x;
        int w = 0, inner_w = 0, ext_w = 0;
        for(int y = 0; y < 3; y++)
        {
            w       = max(w, sizes[y*3+x].w);
            inner_w = max(inner_w, sizes[y*3+x].inner_w);
            ext_w   = max(ext_w, sizes[y*3+x].ext_w);
        }
        cur_x += w + inner_w + ext_w;
        for(int y = 0; y < 3; y++)
            sizes[y*3+x].max_x = cur_x;
    }
    needed_w = cur_x;

    int cur_y = 0;
    for(int y = 0; y < 3; y++)
    {
        for(int x = 0; x < 3; x++)
            sizes[y*3+x].min_y = cur_y;
        int h = 0, inner_h = 0, ext_h = 0;
        for(int x = 0; x < 3; x++)
        {
            h       = max(h, sizes[y*3+x].h);
            inner_h = max(inner_h, sizes[y*3+x].inner_h);
            ext_h   = max(ext_h, sizes[y*3+x].ext_h);
        }
        cur_y += h + inner_h + ext_h;
        for(int x = 0; x < 3; x++)
            sizes[y*3+x].max_y = cur_y;
    }
    needed_h = cur_y;

    r_draw_free_stylebox(c, slot);
    struct r_draw_stylebox* sb = &c->styleboxes[slot];
    sb->definition = *logical_def;
    sb->source_scale = scale;
    sb->w = needed_w;
    sb->h = needed_h;
    sb->side_width [0] = sizes[4].min_x;
    sb->side_width [1] = sizes[4].min_y;
    sb->side_width [2] = needed_w - sizes[4].max_x;
    sb->side_width [3] = needed_h - sizes[4].max_y;
    sb->side_expand[0] = sizes[3].ext_w;
    sb->side_expand[1] = sizes[1].ext_h;
    sb->side_expand[2] = sizes[5].ext_w;
    sb->side_expand[3] = sizes[7].ext_h;
    sb->side_drawn [0] = sizes[3].drawn_w;
    sb->side_drawn [1] = sizes[1].drawn_h;
    sb->side_drawn [2] = sizes[5].drawn_w;
    sb->side_drawn [3] = sizes[7].drawn_h;
    sb->side_inset [0] = (int)roundf((sizes[3].w + sizes[3].pad_w + d->extra_padding[0]) / scale);
    sb->side_inset [1] = (int)roundf((sizes[1].h + sizes[1].pad_h + d->extra_padding[1]) / scale);
    sb->side_inset [2] = (int)roundf((sizes[5].w + sizes[5].pad_w + d->extra_padding[2]) / scale);
    sb->side_inset [3] = (int)roundf((sizes[7].h + sizes[7].pad_h + d->extra_padding[3]) / scale);
    sb->min_w = (int)ceilf((sizes[0].max_x + (sizes[2].max_x - sizes[2].min_x)) / scale);
    sb->min_h = (int)ceilf((sizes[0].max_y + (sizes[6].max_y - sizes[6].min_y)) / scale);
    for(int i = 0; i < 4; i++)
        sb->side_bg_inset[i] = sb->side_width[i] - sb->side_expand[i];

    sb->has_separate_background = d->bg_preserve_aspect_ratio;
    if(d->bg_preserve_aspect_ratio)
    {
        sb->bg_w = d->bg_raster_width;
        sb->bg_h = d->bg_raster_height;
    }
    return true;
}

static uint32_t* rasterize_stylebox(struct r_draw_stylebox* sb)
{
    init_srgb_tables();
    r_draw_stylebox_def def = scale_def(&sb->definition, sb->source_scale);
    r_draw_stylebox_def* d = &def;
    int w = sb->w, h = sb->h;
    rect r = r_shrink_sides((rect){0, 0, w, h}, sb->side_expand[0], sb->side_expand[1], sb->side_expand[2], sb->side_expand[3]);
    uint32_t* canvas = malloc((size_t)w * h * sizeof(uint32_t));
    if(!canvas) return NULL;
    const float big = 4096.0f;
    float cl = sb->side_width[0] - r.x, cr = w - sb->side_width[2] - r.x;
    float ct = sb->side_width[1] - r.y, cb = h - sb->side_width[3] - r.y;
    for(int y = 0; y < h; y++)
    for(int x = 0; x < w; x++)
    {
        float bx = x - r.x, by = y - r.y;
        uint32_t bg = 0x00000000;
        if(d->bg_preserve_aspect_ratio == false)
            bg = background_pixel_at(bx, by, r.w, r.h, d);
        float sx = bx < cl ? bx : bx >= cr ? bx + big : (cl + big * 0.5f);
        float sy = by < ct ? by : by >= cb ? by + big : (ct + big * 0.5f);
        canvas[y * w + x] = finish_pixel(stylebox_pixel_at(bg, sx, sy, r.w + big, r.h + big, d), d);
    }
    return canvas;
}

static uint32_t* rasterize_stylebox_bg(struct r_draw_stylebox* sb)
{
    init_srgb_tables();
    r_draw_stylebox_def def = scale_def(&sb->definition, sb->source_scale);
    uint32_t* bg = malloc((size_t)sb->bg_w * sb->bg_h * sizeof(uint32_t));
    if(!bg) return NULL;
    for(int y = 0; y < sb->bg_h; y++)
    for(int x = 0; x < sb->bg_w; x++)
        bg[y * sb->bg_w + x] = finish_pixel(background_pixel_at(x, y, sb->bg_w, sb->bg_h, &def), &def);
    return bg;
}

static uint64_t stylebox_handle(r_draw* c, struct r_draw_stylebox* sb, bool bg)
{
    uint64_t* handle = bg ? &sb->bg_handle : &sb->handle;
    if(r_draw__still_cached(c, handle) || (bg ? sb->bg_permanent : sb->permanent) || c->cache_full) return *handle;
    uint32_t* pixels = bg ? rasterize_stylebox_bg(sb) : rasterize_stylebox(sb);
    r_draw__cached(c, handle, pixels, bg ? sb->bg_w : sb->w, bg ? sb->bg_h : sb->h);
    free(pixels);
    return *handle;
}


static bool render_stylebox_exact(r_draw* c, int slot, r_frect dst, uint32_t color)
{
    init_srgb_tables();
    struct r_draw_stylebox* sb = &c->styleboxes[slot];
    float s = sb->source_scale / c->scale;
    int w = (int)roundf(dst.w * s), h = (int)roundf(dst.h * s);
    if(w > 256 || h > 256 || w <= 0 || h <= 0) return false;
    struct r_draw_sized_stylebox* e = NULL;
    for(int i = 0; i < c->num_sized_styleboxes && !e; i++)
        if(c->sized_styleboxes[i].slot == slot && c->sized_styleboxes[i].w == w && c->sized_styleboxes[i].h == h)
            e = &c->sized_styleboxes[i];
    if(!e)
    {
        if(c->num_sized_styleboxes >= COUNTOF(c->sized_styleboxes)) return false;
        e = &c->sized_styleboxes[c->num_sized_styleboxes++];
        *e = (struct r_draw_sized_stylebox){ slot, w, h, 0 };
    }
    if(!(e->handle && c->backend.is_cached(c->backend.userptr, e->handle)))
    {
        if(c->cache_full) return false;
        r_draw_stylebox_def def = scale_def(&sb->definition, sb->source_scale);
        r_draw_stylebox_def* d = &def;
        rect r = r_shrink_sides((rect){0, 0, w, h}, sb->side_expand[0], sb->side_expand[1], sb->side_expand[2], sb->side_expand[3]);
        uint32_t* canvas = malloc(w * h * sizeof(uint32_t));
        for(int y = 0; y < h; y++)
        for(int x = 0; x < w; x++)
            canvas[y * w + x] = finish_pixel(stylebox_pixel_at(background_pixel_at(x - r.x, y - r.y, r.w, r.h, d), x - r.x, y - r.y, r.w, r.h, d), d);
        e->handle = 0;
        r_draw__cached(c, &e->handle, canvas, w, h);
        free(canvas);
        if(!e->handle) return false;
    }
    r_draw__emit(c, e->handle, color, (r_frect){ 0, 0, w, h }, dst, c->clip_px);
    return true;
}

// Icons

static inline float sdf_segment(float px, float py, float ax, float ay, float bx, float by)
{
    float pax = px - ax, pay = py - ay, bax = bx - ax, bay = by - ay;
    float h = clamp_unorm((pax * bax + pay * bay) / MAX(bax * bax + bay * bay, 1e-6f));
    return len(pax - bax * h, pay - bay * h);
}

static inline float sdf_arc(float px, float py, float cx, float cy, float r, float a0, float a1)
{
    float dx = px - cx, dy = py - cy;
    float span = fmodf(a1 - a0 + 720.0f, 360.0f);
    if(span == 0.0f) return fabsf(len(dx, dy) - r);
    float a = atan2f(dy, dx) * (360.0f / TAU);
    float rel = fmodf(a - a0 + 720.0f, 360.0f);
    if(rel <= span) return fabsf(len(dx, dy) - r);
    float e0 = a0 * (TAU / 360.0f), e1 = a1 * (TAU / 360.0f);
    return MIN(len(px - (cx + cosf(e0) * r), py - (cy + sinf(e0) * r)),
                   len(px - (cx + cosf(e1) * r), py - (cy + sinf(e1) * r)));
}

static inline float sdf_polygon(float px, float py, const float* pts, int n)
{
    float d = (px - pts[0]) * (px - pts[0]) + (py - pts[1]) * (py - pts[1]);
    float s = 1.0f;
    for(int i = 0, j = n - 1; i < n; j = i, i++)
    {
        float ex = pts[j*2] - pts[i*2], ey = pts[j*2+1] - pts[i*2+1];
        float wx = px - pts[i*2], wy = py - pts[i*2+1];
        float h = clamp_unorm((wx * ex + wy * ey) / MAX(ex * ex + ey * ey, 1e-6f));
        float bx = wx - ex * h, by = wy - ey * h;
        d = MIN(d, bx * bx + by * by);
        bool c0 = py >= pts[i*2+1], c1 = py < pts[j*2+1], c2 = ex * wy > ey * wx;
        if((c0 && c1 && c2) || (!c0 && !c1 && !c2)) s = -s;
    }
    return s * sqrtf(d);
}

static inline float sdf_rect(float px, float py, float x, float y, float w, float h, float r)
{
    return sdf_rounded_box(px - x - w / 2, py - y - h / 2, w, h, r, r, r, r);
}

static int icon_op_size(const float* op)
{
    switch((int)op[0])
    {
        case R_ICON_OP_WIDTH:    return 2;
        case R_ICON_OP_POLYLINE: return 2 + (int)op[1] * 2;
        case R_ICON_OP_ARC:      return 6;
        case R_ICON_OP_RECT:     return 6;
        case R_ICON_OP_COLOR:    return 5;
        default:                 return 0;
    }
}

static bool icon_has_color(const float* op)
{
    for(int n; (n = icon_op_size(op)); op += n)
        if((int)op[0] == R_ICON_OP_COLOR) return true;
    return false;
}

static void icon_pixel(const float* op, float x, float y, float px, float out[4])
{
    float color[4] = { 1, 1, 1, 1 }, cov = 0.0f, width = 2.0f;
    out[0] = out[1] = out[2] = out[3] = 0.0f;
    for(int n; ; op += n)
    {
        n = icon_op_size(op);
        int code = n ? (int)op[0] : R_ICON_OP_END;
        const float* a = op + 1;
        if(code == R_ICON_OP_COLOR || code == R_ICON_OP_END)
        {
            float k = cov * color[3];
            for(int i = 0; i < 4; i++) out[i] = out[i] * (1.0f - k) + (i < 3 ? color[i] : 1.0f) * k;
            if(code == R_ICON_OP_END) return;
            memcpy(color, a, sizeof(color));
            cov = 0.0f;
            continue;
        }
        if(code == R_ICON_OP_WIDTH) { width = a[0]; continue; }
        bool fill = width <= 0.0f;
        float d;
        if(code == R_ICON_OP_POLYLINE)
        {
            int count = (int)a[0];
            if(fill) d = sdf_polygon(x, y, a + 1, count);
            else
            {
                d = 1e9f;
                for(int i = 0; i + 1 < count; i++)
                    d = MIN(d, sdf_segment(x, y, a[1 + i*2], a[2 + i*2], a[3 + i*2], a[4 + i*2]));
            }
        }
        else if(code == R_ICON_OP_ARC) d = fill ? len(x - a[0], y - a[1]) - a[2] : sdf_arc(x, y, a[0], a[1], a[2], a[3], a[4]);
        else                           d = fill ? sdf_rect(x, y, a[0], a[1], a[2], a[3], a[4]) : fabsf(sdf_rect(x, y, a[0], a[1], a[2], a[3], a[4]));
        if(!fill) d -= width * 0.5f;
        cov = MAX(cov, clamp_unorm(0.5f - d / px));
    }
}

bool r_draw_upload_icon(r_draw* c, int slot, r_draw_icon_def* icon)
{
    if(slot < 0 || slot >= COUNTOF(c->icons)) return false;
    c->icons[slot] = *icon;
    return true;
}

rect r_draw_icon_get_rect(r_draw* c, int slot, rect outer)
{
    int s = MIN(outer.w, outer.h);
    rect r = {.w = s, .h = s, .id = outer.id};
    r_align(R_CENTER, outer, &r);
    return r;
}

uint64_t r_draw__cache_icon(r_draw* c, int icon, int size, bool* colored)
{
    const float* ops = c->icons[icon].ops;
    float px = 24.0f / size, p[4];
    *colored = icon_has_color(ops);
    uint64_t handle;
    if(*colored)
    {
        uint32_t* pixels = malloc(size * size * 4);
        for(int y = 0; y < size; y++)
        for(int x = 0; x < size; x++)
        {
            icon_pixel(ops, (x + 0.5f) * px, (y + 0.5f) * px, px, p);
            uint32_t argb = 0;
            for(int i = 0; i < 4; i++) argb |= (uint32_t)(clamp_unorm(p[i]) * 255.0f + 0.5f) << (i == 3 ? 24 : 16 - i * 8);
            pixels[y * size + x] = argb;
        }
        handle = c->backend.cache_argb(c->backend.userptr, size, size, pixels, size * 4, false);
        free(pixels);
    }
    else
    {
        uint8_t* pixels = malloc(size * size);
        for(int y = 0; y < size; y++)
        for(int x = 0; x < size; x++)
        {
            icon_pixel(ops, (x + 0.5f) * px, (y + 0.5f) * px, px, p);
            pixels[y * size + x] = (uint8_t)(clamp_unorm(p[3]) * 255.0f + 0.5f);
        }
        handle = c->backend.cache_monochrome(c->backend.userptr, size, size, pixels, size, false);
        free(pixels);
    }
    return handle;
}

void r_stylebox_set_raised_borders_simple(r_draw_stylebox_def* box, float angle, uint32_t light_color, uint32_t ambient_color, float border_thickness, float border_radius, float blur)
{
    static const float normals[4][2] = { {-1, 0}, {0, -1}, {1, 0}, {0, 1} };
    float a = angle * TAU / 360.0f;
    float lx = sinf(a), ly = -cosf(a);
    for(int i = 0; i < 4; i++)
    {
        float facing = clamp_unorm(normals[i][0] * lx + normals[i][1] * ly);
        box->border_gradient[i].base_color = lerp_col(ambient_color, light_color, facing);
        box->border_gradient[i].use_gradient = false;
        box->border_thickness[i] = border_thickness;
        box->border_blur[i] = blur;
        box->corner_radius[i] = border_radius;
    }
}

void r_draw_free_stylebox(r_draw* c, int slot)
{
    struct r_draw_stylebox* sb = &c->styleboxes[slot];
    r_draw__invalidate(c, &sb->handle);
    r_draw__invalidate(c, &sb->bg_handle);
    for(int i = 0; i < c->num_sized_styleboxes; i++)
        if(c->sized_styleboxes[i].slot == slot)
        {
            r_draw__invalidate(c, &c->sized_styleboxes[i].handle);
            c->sized_styleboxes[i--] = c->sized_styleboxes[--c->num_sized_styleboxes];
        }
    *sb = (struct r_draw_stylebox){0};
}

static uint32_t* premult_copy(const uint8_t* pixels, int w, int h, int pitch)
{
    uint32_t* tmp = malloc(w * h * 4);
    for(int y = 0; y < h; y++)
    for(int x = 0; x < w; x++)
    {
        uint32_t v; memcpy(&v, pixels + y * pitch + x * 4, 4);
        tmp[y * w + x] = premult(v);
    }
    return tmp;
}

bool r_draw_upload_stylebox(r_draw* c, int slot, uint8_t* pixels, int w, int h, int pitch, int left, int right, int top, int bottom)
{
    if(slot <= 0 || slot >= RCUT_DRAW_MAX_STYLEBOXES || w <= 0 || h <= 0) return false;
    r_draw_free_stylebox(c, slot);
    uint32_t* tmp = premult_copy(pixels, w, h, pitch);
    uint64_t handle = c->backend.cache_argb(c->backend.userptr, w, h, tmp, w * 4, true);
    free(tmp);
    if(!handle) return false;
    struct r_draw_stylebox* sb = &c->styleboxes[slot];
    sb->permanent = true;
    sb->handle = handle;
    sb->w = w;
    sb->h = h;
    int sides[4] = { left, top, right, bottom };
    for(int i = 0; i < 4; i++)
    {
        sb->side_width[i] = sides[i];
        sb->side_inset[i] = sides[i];
        sb->side_bg_inset[i] = sides[i];
    }
    sb->min_w = left + right;
    sb->min_h = top + bottom;
    sb->source_scale = 1.0f;
    return true;
}

bool r_draw_upload_stylebox_fill(r_draw* c, int slot, uint8_t* pixels, int w, int h, int pitch)
{
    if(slot <= 0 || slot >= RCUT_DRAW_MAX_STYLEBOXES || w <= 0 || h <= 0) return false;
    uint32_t* tmp = premult_copy(pixels, w, h, pitch);
    uint64_t handle = c->backend.cache_argb(c->backend.userptr, w, h, tmp, w * 4, true);
    free(tmp);
    if(!handle) return false;
    struct r_draw_stylebox* sb = &c->styleboxes[slot];
    r_draw__invalidate(c, &sb->bg_handle);
    sb->bg_handle = handle;
    sb->bg_permanent = true;
    sb->bg_w = w;
    sb->bg_h = h;
    sb->has_separate_background = true;
    return true;
}
