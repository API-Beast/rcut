#include "internal.h"
#if defined(RCUT_USE_HARFBUZZ) || defined(RCUT_USE_HB_MINI)
#ifdef RCUT_USE_HB_MINI
  #include <hb_mini.h>
#else
  #include <harfbuzz/hb.h>
  #include <harfbuzz/hb-ot.h>
  #include <harfbuzz/hb-raster.h>
#endif

struct r_draw_face_inst
{
    hb_font_t* font;
    hb_face_t* face;
};

struct font_state
{
    hb_buffer_t*        buffer;
    hb_raster_paint_t*  paint;
    hb_raster_image_t*  image;
    r_draw_glyph_pos*   pos; int cap_pos;
};

static struct font_state* state(r_draw* c)
{
    if(!c->font_state)
    {
        struct font_state* s = calloc(1, sizeof(*s));
        s->buffer = hb_buffer_create();
        s->paint  = hb_raster_paint_create_or_fail();
        c->font_state = s;
    }
    return c->font_state;
}

void __draw_face_free_state(r_draw* c)
{
    struct font_state* s = c->font_state;
    if(!s) return;
    if(s->image) hb_raster_paint_recycle_image(s->paint, s->image);
    if(s->paint) hb_raster_paint_destroy(s->paint);
    hb_buffer_destroy(s->buffer);
    free(s->pos);
    free(s);
    c->font_state = NULL;
}

struct r_draw_face* __draw_face_load(const char* path)
{
    hb_blob_t* blob = hb_blob_create_from_file_or_fail(path);
    if(!blob) return NULL;
    hb_face_t* face = hb_face_create(blob, 0);
    hb_blob_destroy(blob);
    if(!face || hb_face_get_glyph_count(face) == 0)
    {
        hb_face_destroy(face);
        return NULL;
    }
    return (struct r_draw_face*)face;
}

void __draw_face_free(struct r_draw_face* face)
{
    hb_face_destroy((hb_face_t*)face);
}

int __draw_face_weight(struct r_draw_face* face)
{
    hb_font_t* font = hb_font_create((hb_face_t*)face);
    int weight = (int)hb_style_get_value(font, HB_STYLE_TAG_WEIGHT);
    hb_font_destroy(font);
    return weight > 0 ? weight : 400;
}

struct r_draw_face_inst* __draw_face_inst(struct r_draw_face* face_, int px, int weight, int synth_weight, bool italic)
{
    hb_face_t* face = (hb_face_t*)face_;
    hb_font_t* font = hb_font_create(face);
    if(!font) return NULL;
    hb_font_set_scale(font, px * 64, px * 64);
    hb_font_set_ppem (font, px, px);

    // Inspect the variation axes so we lean on the face's own weight/slant
    // before falling back to synthesis.
    bool  has_wght = false, has_slnt = false, has_ital = false;
    float wght_min = 0, wght_max = 0, slnt_min = 0, slnt_max = 0;
    if(hb_ot_var_get_axis_count(face))
    {
        hb_ot_var_axis_info_t axes[16];
        unsigned int n = sizeof(axes) / sizeof(axes[0]);
        hb_ot_var_get_axis_infos(face, 0, &n, axes);
        for(unsigned int i = 0; i < n; i++)
        {
            switch(axes[i].tag)
            {
                case HB_TAG('w','g','h','t'):
                    has_wght = true; wght_min = axes[i].min_value; wght_max = axes[i].max_value; break;
                case HB_TAG('s','l','n','t'):
                    has_slnt = true; slnt_min = axes[i].min_value; slnt_max = axes[i].max_value; break;
                case HB_TAG('i','t','a','l'):
                    has_ital = true; break;
            }
        }
    }

    hb_variation_t vars[2];
    int nvars = 0;
    if(has_wght && weight != 0)
    {
        float w = (float)weight;
        if(w < wght_min) w = wght_min;
        if(w > wght_max) w = wght_max;
        vars[nvars++] = (hb_variation_t){ HB_TAG('w','g','h','t'), w };
    }
    if(italic && has_ital)
        vars[nvars++] = (hb_variation_t){ HB_TAG('i','t','a','l'), 1.0f };
    else if(italic && has_slnt)
    {
        float s = -12.0f; // slnt is in degrees; negative leans forward.
        if(s < slnt_min) s = slnt_min;
        if(s > slnt_max) s = slnt_max;
        vars[nvars++] = (hb_variation_t){ HB_TAG('s','l','n','t'), s };
    }
    if(nvars) hb_font_set_variations(font, vars, nvars);

    // Synthesize only the features the face couldn't supply natively.
    if(italic && !has_ital && !has_slnt)
        hb_font_set_synthetic_slant(font, 0.207f); // ~12deg, the usual oblique.
    if(synth_weight != 0 && !has_wght)
    {
        float embolden = px * 0.03f * synth_weight / 300.0f;
        hb_font_set_synthetic_bold(font, embolden, embolden, true);
    }
    struct r_draw_face_inst* f = malloc(sizeof(*f));
    *f = (struct r_draw_face_inst){ font, face };
    return f;
}

void __draw_face_inst_free(struct r_draw_face_inst* f)
{
    hb_font_destroy(f->font);
    free(f);
}

void __draw_face_inst_resize(struct r_draw_face_inst* f, int old_px, int px)
{
    hb_font_t* font = f->font;
    hb_font_set_scale(font, px * 64, px * 64);
    hb_font_set_ppem (font, px, px);
    float x, y; hb_bool_t in_place;
    hb_font_get_synthetic_bold(font, &x, &y, &in_place);
    if((x || y) && old_px > 0) hb_font_set_synthetic_bold(font, x * px / old_px, y * px / old_px, in_place);
}

bool __draw_face_glyph(struct r_draw_face_inst* f, uint32_t codepoint, uint32_t* gid)
{
    return hb_font_get_nominal_glyph(f->font, codepoint, gid);
}

int __draw_face_advance(struct r_draw_face_inst* f, uint32_t gid, bool* empty)
{
    hb_glyph_extents_t extents;
    *empty = !hb_font_get_glyph_extents(f->font, gid, &extents) || extents.width == 0 || extents.height == 0;
    return hb_font_get_glyph_h_advance(f->font, gid);
}

void __draw_face_v_metrics(struct r_draw_face_inst* f, int* ascender, int* descender, int* line_gap)
{
    hb_font_extents_t fe;
    hb_font_get_h_extents(f->font, &fe);
    *ascender = fe.ascender; *descender = fe.descender; *line_gap = fe.line_gap;
}

int __draw_face_cap_height(struct r_draw_face_inst* f)
{
    hb_codepoint_t gid;
    hb_glyph_extents_t extents;
    if(!hb_font_get_nominal_glyph(f->font, 'H', &gid) || !hb_font_get_glyph_extents(f->font, gid, &extents)) return 0;
    return extents.y_bearing;
}

int __draw_face_shape(r_draw* c, struct r_draw_face_inst* f, const char* text, int len, int start, int n, r_draw_glyph_pos** out)
{
    struct font_state* s = state(c);
    hb_buffer_clear_contents(s->buffer);
    hb_buffer_add_utf8(s->buffer, text, len, (unsigned)start, n);
    hb_buffer_guess_segment_properties(s->buffer);
    hb_shape(f->font, s->buffer, NULL, 0);
    unsigned count;
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(s->buffer, &count);
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(s->buffer, &count);
    s->pos = __draw_grow(s->pos, &s->cap_pos, (int)count, sizeof(r_draw_glyph_pos));
    for(unsigned i = 0; i < count; i++)
        s->pos[i] = (r_draw_glyph_pos){ info[i].codepoint, info[i].cluster, pos[i].x_advance, pos[i].x_offset, pos[i].y_offset };
    *out = s->pos;
    return (int)count;
}

bool __draw_face_raster(r_draw* c, struct r_draw_face_inst* f, uint32_t gid, r_draw_glyph_bitmap* out)
{
    struct font_state* s = state(c);
    hb_font_t* font = f->font;
    hb_glyph_extents_t glyph_extents;
    if(!s->paint || !hb_font_get_glyph_extents(font, gid, &glyph_extents)) return false;
    if(s->image) { hb_raster_paint_recycle_image(s->paint, s->image); s->image = NULL; }

    hb_raster_paint_reset(s->paint);
    hb_raster_paint_set_transform(s->paint, 1.0, 0.0, 0.0, -1.0, 0.0, 0.0);
    hb_raster_paint_set_scale_factor(s->paint, 64.0, 64.0);
    hb_raster_paint_set_glyph_extents(s->paint, &glyph_extents);
    hb_raster_paint_set_foreground(s->paint, 0xFFFFFFFF);
    hb_raster_paint_glyph(s->paint, font, gid);
    s->image = hb_raster_paint_render(s->paint);
    if(!s->image) return false;

    hb_raster_extents_t ext;
    hb_raster_image_get_extents(s->image, &ext);
    *out = (r_draw_glyph_bitmap){
        .pixels  = hb_raster_image_get_buffer(s->image),
        .x = ext.x_origin, .y = ext.y_origin, .w = (int)ext.width, .h = (int)ext.height, .stride = (int)ext.stride, .bpp = 4,
        .colored = hb_ot_color_glyph_has_paint(f->face, gid) || hb_ot_color_glyph_get_layers(f->face, gid, 0, NULL, NULL) > 0 || hb_ot_color_has_png(f->face),
    };
    return true;
}
#endif
