#ifndef RCUT_DRAW_INTERNAL_H
#define RCUT_DRAW_INTERNAL_H
#include "../rcut_draw.h"
#include "../core/internal.h"
#include <stdlib.h>

typedef struct {
    uint8_t *start;
    uint8_t *cur;
    uint8_t *end;
} arena_t;

static inline void* arena_push(arena_t* a, int len)
{
    assert(a->cur+len < a->end);
    void* retval = a->cur;
    a->cur += len;
    return retval;
}

static inline rect __draw_to_pixels(rect r, float s)
{
    int x0 = (int)floorf(r.x * s + 0.5f), y0 = (int)floorf(r.y * s + 0.5f);
    int x1 = (int)floorf((r.x + r.w) * s + 0.5f), y1 = (int)floorf((r.y + r.h) * s + 0.5f);
    return (rect){ x0, y0, x1 - x0, y1 - y0, r.id };
}

static inline bool __draw_invert(float m[2][3], float inv[2][3])
{
    float det = m[0][0] * m[1][1] - m[0][1] * m[1][0];
    if(det == 0.0f) return false;
    inv[0][0] =  m[1][1] / det; inv[0][1] = -m[0][1] / det;
    inv[1][0] = -m[1][0] / det; inv[1][1] =  m[0][0] / det;
    inv[0][2] = -(inv[0][0] * m[0][2] + inv[0][1] * m[1][2]);
    inv[1][2] = -(inv[1][0] * m[0][2] + inv[1][1] * m[1][2]);
    return true;
}

static inline uint32_t __draw_premult_color(uint32_t color)
{
    uint32_t a = color >> 24;
    return (a << 24) | ((((color >> 16) & 0xFF) * a / 255) << 16) | ((((color >> 8) & 0xFF) * a / 255) << 8) | ((color & 0xFF) * a / 255);
}

void     __draw_emit(r_draw* c, uint64_t handle, uint32_t color, r_frect src, r_frect dst, rect clip);
bool     __draw_still_cached(r_draw* c, uint64_t* handle);
uint64_t __draw_cached(r_draw* c, uint64_t* handle, const uint32_t* pixels, int w, int h);
void     __draw_invalidate(r_draw* c, uint64_t* handle);

void     __draw_theme_flush(r_draw* c);

uint64_t __draw_cache_icon(r_draw* c, int icon, int size, bool* colored);
void     __draw_rescale_fonts(r_draw* c);

typedef struct r_draw_glyph_pos    { uint32_t glyph, cluster; int32_t x_advance, x_offset, y_offset; } r_draw_glyph_pos;
typedef struct r_draw_glyph_bitmap { const uint8_t* pixels; int x, y, w, h, stride, bpp; bool colored; } r_draw_glyph_bitmap;

// Font backend: font_hb.c (HarfBuzz/hb_mini) or font_stb.c (stb_truetype). Metrics in 26.6.
struct r_draw_face*      __draw_face_load(const char* path);
void                     __draw_face_free(struct r_draw_face* face);
int                      __draw_face_weight(struct r_draw_face* face);
struct r_draw_face_inst* __draw_face_inst(struct r_draw_face* face, int px, int weight, int synth_weight, bool italic);
void                     __draw_face_inst_free(struct r_draw_face_inst* f);
void                     __draw_face_inst_resize(struct r_draw_face_inst* f, int old_px, int px);
bool                     __draw_face_glyph(struct r_draw_face_inst* f, uint32_t codepoint, uint32_t* gid);
int                      __draw_face_advance(struct r_draw_face_inst* f, uint32_t gid, bool* empty);
void                     __draw_face_v_metrics(struct r_draw_face_inst* f, int* ascender, int* descender, int* line_gap);
int                      __draw_face_cap_height(struct r_draw_face_inst* f); // 26.6, 0 if unknown.
int                      __draw_face_shape(r_draw* c, struct r_draw_face_inst* f, const char* text, int len, int start, int n, r_draw_glyph_pos** out);
bool                     __draw_face_raster(r_draw* c, struct r_draw_face_inst* f, uint32_t gid, r_draw_glyph_bitmap* out);
void                     __draw_face_free_state(r_draw* c);

static inline uint32_t __draw_decode_utf8(const char** str)
{
    const uint8_t* s = (const uint8_t*)*str;

    uint32_t codepoint;
         if((s[0] & 0x80) == 0)   { *str += 1; return s[0]; }
    else if((s[0] & 0xE0) == 0xC0){ *str += 2; return ((s[0] & 0x1F) << 6)  |  (s[1] & 0x3F); }
    else if((s[0] & 0xF0) == 0xE0){ *str += 3; return ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6)  |  (s[2] & 0x3F); }

    *str += 4;
    return ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
}

static inline void* __draw_grow(void* p, int* cap, int need, int size)
{
    if(need <= *cap) return p;
    while(*cap < need) *cap = *cap ? *cap * 2 : 256;
    return realloc(p, (size_t)*cap * size);
}

bool __draw_glyph_pixels(r_draw* c, int glyph_idx);
void __draw_clear_glyph_cache(r_draw* c);
void __draw_free_text(r_draw* c);
void __draw_render_text(r_draw* c, r_drawable* d, const r_draw_style_rule* rule, rect bounds, r_alignment align, rect clip, int wrap_width);

#endif
