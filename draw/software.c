#include "internal.h"

static bool     __sw_renderer_is_cached       (void* userptr, uint64_t handle);
static void     __sw_renderer_invalidate_cache(void* userptr, uint64_t handle);
static uint64_t __sw_renderer_cache_argb      (void* userptr, int w, int h, const uint32_t* pixels, int stride, bool permanent);
static uint64_t __sw_renderer_cache_monochrome(void* userptr, int w, int h, const uint8_t* pixels, int stride, bool permanent);
static void     __sw_renderer_submit          (void* userptr, const r_draw_quad* quads, int num_quads);

r_draw_backend r_draw_software_backend(r_draw_software* sw)
{
    return (r_draw_backend){
        .userptr          = sw,
        .cache_argb       = __sw_renderer_cache_argb,
        .cache_monochrome = __sw_renderer_cache_monochrome,
        .submit           = __sw_renderer_submit,
        .is_cached        = __sw_renderer_is_cached,
        .invalidate_cache = __sw_renderer_invalidate_cache,
    };
}

#define POS_BITS 38
#define PERM_BIT (1ull << 38)
#define MONO_BIT (1ull << 39)
static inline uint64_t entry_pos(uint64_t handle) { return handle & ((1ull << POS_BITS) - 1); }
static inline int      entry_w  (uint64_t handle) { return (int)((handle >> 40) & 0xFFF); }
static inline int      entry_h  (uint64_t handle) { return (int)(handle >> 52); }

static bool __sw_renderer_is_cached(void* userptr, uint64_t handle)
{
    r_draw_software* sw = userptr;
    uint64_t pos = entry_pos(handle);
    if(handle & PERM_BIT) return pos < (uint64_t)sw->num_permanent && sw->permanent[pos];
    if(!handle || sw->write_pos > pos + sw->cache_size) return false;
    if(pos < sw->frame_oldest) sw->frame_oldest = pos;
    return true;
}

static uint32_t* sw_alloc(r_draw_software* sw, int w, int h, uint64_t n, uint64_t* handle)
{
    uint64_t pos = sw->write_pos;
    if(pos % sw->cache_size + n > sw->cache_size) pos += sw->cache_size - pos % sw->cache_size;
    if(w > 4095 || h > 4095 || pos + n > sw->frame_oldest + sw->cache_size) { sw->full = true; return NULL; }
    sw->write_pos = pos + n;
    *handle = pos | ((uint64_t)w << 40) | ((uint64_t)h << 52);
    return sw->cache + pos % sw->cache_size;
}

static void* sw_alloc_permanent(r_draw_software* sw, int w, int h, size_t bytes, uint64_t* handle)
{
    if(w > 4095 || h > 4095) return NULL;
    int slot = 0;
    while(slot < sw->num_permanent && sw->permanent[slot]) slot++;
    if(slot == sw->num_permanent)
    {
        void** grown = realloc(sw->permanent, (size_t)(sw->num_permanent + 1) * sizeof(void*));
        if(!grown) return NULL;
        sw->permanent = grown;
        sw->permanent[sw->num_permanent++] = NULL;
    }
    sw->permanent[slot] = malloc(bytes);
    if(sw->permanent[slot]) *handle = (uint64_t)slot | PERM_BIT | ((uint64_t)w << 40) | ((uint64_t)h << 52);
    return sw->permanent[slot];
}

static void __sw_renderer_invalidate_cache(void* userptr, uint64_t handle)
{
    r_draw_software* sw = userptr;
    uint64_t pos = entry_pos(handle);
    if(!(handle & PERM_BIT) || pos >= (uint64_t)sw->num_permanent) return;
    free(sw->permanent[pos]);
    sw->permanent[pos] = NULL;
}

static uint64_t __sw_renderer_cache_argb(void* userptr, int w, int h, const uint32_t* pixels, int stride, bool permanent)
{
    uint64_t handle = 0;
    uint32_t* dst = permanent ? sw_alloc_permanent(userptr, w, h, (size_t)w * h * 4, &handle) : sw_alloc(userptr, w, h, (uint64_t)w * h, &handle);
    if(dst)
    for(int y = 0; y < h; y++)
        memcpy(dst + y * w, (const uint8_t*)pixels + y * stride, w * 4);
    return handle;
}

static uint64_t __sw_renderer_cache_monochrome(void* userptr, int w, int h, const uint8_t* pixels, int stride, bool permanent)
{
    uint64_t handle = 0;
    uint8_t* dst = permanent ? sw_alloc_permanent(userptr, w, h, (size_t)w * h, &handle)
                             : (uint8_t*)sw_alloc(userptr, w, h, ((uint64_t)w * h + 3) / 4, &handle);
    if(dst)
    for(int y = 0; y < h; y++)
        memcpy(dst + y * w, pixels + y * stride, w);
    return handle ? handle | MONO_BIT : 0;
}

static inline uint32_t mul_lanes(uint32_t x, uint32_t a)
{
    x = x * a + 0x00800080;
    return ((x + ((x >> 8) & 0x00FF00FF)) >> 8) & 0x00FF00FF;
}

static inline uint32_t scale(uint32_t p, uint32_t a)
{
    return mul_lanes(p & 0x00FF00FF, a) | (mul_lanes((p >> 8) & 0x00FF00FF, a) << 8);
}

static inline uint32_t tint(uint32_t p, uint32_t c)
{
    uint32_t r = 0;
    for(int s = 0; s < 32; s += 8)
    {
        uint32_t t = ((p >> s) & 0xFF) * ((c >> s) & 0xFF) + 128;
        r |= ((t + (t >> 8)) >> 8) << s;
    }
    return r;
}

static inline uint32_t over(uint32_t d, uint32_t s)
{
    uint32_t a = s >> 24;
    if(a == 255) return s;
    if(s == 0) return d;
    return s + scale(d, 255 - a);
}

static inline uint32_t blend_texel(uint32_t d, const uint32_t* e, int ew, bool mono, int sx, int sy, uint32_t c)
{
    if(mono)
    {
        uint32_t a = ((const uint8_t*)e)[sy * ew + sx];
        return a ? over(d, a == 255 ? c : scale(c, a)) : d;
    }
    uint32_t p = e[sy * ew + sx];
    return p ? over(d, c == 0xFFFFFFFF ? p : tint(p, c)) : d;
}

static void blit_transformed(r_draw_software* sw, const r_draw_quad* q, const uint32_t* e, int ew, int eh, bool mono)
{
    const r_draw_transform* t = q->transform;
    float m[2][3], inv[2][3];
    memcpy(m, t->m, sizeof(m));
    if(!__draw_invert(m, inv)) return;
    float x0 = 1e30f, y0 = 1e30f, x1 = -1e30f, y1 = -1e30f;
    for(int i = 0; i < 4; i++)
    {
        float px = q->dst.x + (i & 1 ? q->dst.w : 0), py = q->dst.y + (i & 2 ? q->dst.h : 0);
        float x = m[0][0] * px + m[0][1] * py + m[0][2], y = m[1][0] * px + m[1][1] * py + m[1][2];
        x0 = MIN(x0, x); y0 = MIN(y0, y); x1 = MAX(x1, x); y1 = MAX(y1, y);
    }
    int ix0 = MAX(MAX((int)floorf(x0), t->clip.x), sw->x), ix1 = MIN(MIN((int)ceilf(x1), t->clip.x + t->clip.w), sw->x + sw->w);
    int iy0 = MAX(MAX((int)floorf(y0), t->clip.y), sw->y), iy1 = MIN(MIN((int)ceilf(y1), t->clip.y + t->clip.h), sw->y + sw->h);
    float kx = q->src.w / q->dst.w, ky = q->src.h / q->dst.h;
    for(int y = iy0; y < iy1; y++)
    {
        uint32_t* row = &sw->pixels[(y - sw->y) * sw->stride];
        float u = inv[0][0] * (ix0 + 0.5f) + inv[0][1] * (y + 0.5f) + inv[0][2];
        float v = inv[1][0] * (ix0 + 0.5f) + inv[1][1] * (y + 0.5f) + inv[1][2];
        for(int x = ix0; x < ix1; x++, u += inv[0][0], v += inv[1][0])
        {
            if(u < q->dst.x || v < q->dst.y || u >= q->dst.x + q->dst.w || v >= q->dst.y + q->dst.h) continue;
            int sx = (int)(q->src.x + (u - q->dst.x) * kx), sy = (int)(q->src.y + (v - q->dst.y) * ky);
            sx = sx < 0 ? 0 : sx >= ew ? ew - 1 : sx;
            sy = sy < 0 ? 0 : sy >= eh ? eh - 1 : sy;
            row[x - sw->x] = blend_texel(row[x - sw->x], e, ew, mono, sx, sy, q->color);
        }
    }
}

static void __sw_renderer_submit(void* userptr, const r_draw_quad* quads, int num_quads)
{
    r_draw_software* sw = userptr;
    if(sw->pixels)
    for(int y = 0; y < sw->h; y++)
    for(int x = 0; x < sw->w; x++)
        sw->pixels[y * sw->stride + x] = sw->clear_color;
    int* xs = sw->columns;
    for(int i = 0; i < num_quads && sw->pixels; i++)
    {
        const r_draw_quad* q = &quads[i];
        int ew = entry_w(q->handle), eh = entry_h(q->handle);
        bool mono = q->handle & MONO_BIT;
        const uint32_t* e = q->handle & PERM_BIT ? sw->permanent[entry_pos(q->handle)] : sw->cache + entry_pos(q->handle) % sw->cache_size;
        if(q->transform) { blit_transformed(sw, q, e, ew, eh, mono); continue; }
        float dx = q->dst.x - sw->x, dy = q->dst.y - sw->y;
        int x0 = MAX((int)ceilf(dx - 0.5f), 0), x1 = MIN((int)ceilf(dx + q->dst.w - 0.5f), sw->w);
        int y0 = MAX((int)ceilf(dy - 0.5f), 0), y1 = MIN((int)ceilf(dy + q->dst.h - 0.5f), sw->h);
        float kx = q->src.w / q->dst.w, ky = q->src.h / q->dst.h;
        for(int x = x0; x < x1; x++)
        {
            int sx = (int)(q->src.x + (x + 0.5f - dx) * kx);
            xs[x - x0] = sx < 0 ? 0 : sx >= ew ? ew - 1 : sx;
        }
        uint32_t c = q->color;
        for(int y = y0; y < y1; y++)
        {
            int sy = (int)(q->src.y + (y + 0.5f - dy) * ky);
            sy = sy < 0 ? 0 : sy >= eh ? eh - 1 : sy;
            uint32_t* d = &sw->pixels[y * sw->stride + x0];
            int n = x1 - x0;
            if(mono)
            {
                const uint8_t* row = (const uint8_t*)e + sy * ew;
                for(int x = 0; x < n; x++)
                {
                    uint32_t a = row[xs[x]];
                    if(a) d[x] = over(d[x], a == 255 ? c : scale(c, a));
                }
            }
            else
            {
                const uint32_t* row = e + sy * ew;
                if(c == 0xFFFFFFFF) for(int x = 0; x < n; x++) d[x] = over(d[x], row[xs[x]]);
                else                for(int x = 0; x < n; x++) { uint32_t p = row[xs[x]]; if(p) d[x] = over(d[x], tint(p, c)); }
            }
        }
    }
    if(sw->full) sw->write_pos += sw->cache_size;
    sw->full = false;
    sw->frame_oldest = sw->write_pos;
}

void r_draw_software_init(r_draw_software* sw, int w, int h, int cache_size)
{
    *sw = (r_draw_software){ .cache = malloc(cache_size * 4), .cache_size = cache_size };
    if(w > 0 && h > 0) sw->buffer = calloc((size_t)w * h, 4);
    r_draw_software_set_target(sw, sw->buffer, w, h, w, 0, 0);
}

void r_draw_software_set_target(r_draw_software* sw, uint32_t* pixels, int w, int h, int stride, int x, int y)
{
    if(w > sw->num_columns)
    {
        free(sw->columns);
        sw->columns = malloc(w * sizeof(int));
        sw->num_columns = sw->columns ? w : 0;
    }
    sw->pixels = sw->columns ? pixels : NULL;
    sw->w = w; sw->h = h; sw->stride = stride;
    sw->x = x; sw->y = y;
}

void r_draw_software_free(r_draw_software* sw)
{
    free(sw->buffer);
    free(sw->cache);
    free(sw->columns);
    for(int i = 0; i < sw->num_permanent; i++) free(sw->permanent[i]);
    free(sw->permanent);
    *sw = (r_draw_software){0};
}
