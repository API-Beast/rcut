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

static inline rect r_draw__to_pixels(rect r, float s)
{
    int x0 = (int)floorf(r.x * s + 0.5f), y0 = (int)floorf(r.y * s + 0.5f);
    int x1 = (int)floorf((r.x + r.w) * s + 0.5f), y1 = (int)floorf((r.y + r.h) * s + 0.5f);
    return (rect){ x0, y0, x1 - x0, y1 - y0, r.id };
}

static inline bool r_draw__invert(float m[2][3], float inv[2][3])
{
    float det = m[0][0] * m[1][1] - m[0][1] * m[1][0];
    if(det == 0.0f) return false;
    inv[0][0] =  m[1][1] / det; inv[0][1] = -m[0][1] / det;
    inv[1][0] = -m[1][0] / det; inv[1][1] =  m[0][0] / det;
    inv[0][2] = -(inv[0][0] * m[0][2] + inv[0][1] * m[1][2]);
    inv[1][2] = -(inv[1][0] * m[0][2] + inv[1][1] * m[1][2]);
    return true;
}

static inline uint32_t r_draw__premult_color(uint32_t color)
{
    uint32_t a = color >> 24;
    return (a << 24) | ((((color >> 16) & 0xFF) * a / 255) << 16) | ((((color >> 8) & 0xFF) * a / 255) << 8) | ((color & 0xFF) * a / 255);
}

void     r_draw__emit(r_draw* c, uint64_t handle, uint32_t color, r_frect src, r_frect dst, rect clip);
bool     r_draw__still_cached(r_draw* c, uint64_t* handle);
uint64_t r_draw__cached(r_draw* c, uint64_t* handle, const uint32_t* pixels, int w, int h);
void     r_draw__invalidate(r_draw* c, uint64_t* handle);

void     r_theme__flush(r_draw* c);

uint64_t r_draw__cache_icon(r_draw* c, int icon, int size, bool* colored);
void     r_draw__rescale_fonts(r_draw* c);

bool r_draw__glyph_pixels(r_draw* c, int glyph_idx);
void r_draw__clear_glyph_cache(r_draw* c);
void r_draw__free_text(r_draw* c);
void r_draw__render_text(r_draw* c, r_drawable* d, const r_draw_style_rule* rule, rect bounds, r_alignment align, rect clip, int wrap_width);

#endif
