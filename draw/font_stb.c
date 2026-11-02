#include "internal.h"
#ifdef RCUT_USE_STB_TRUETYPE
#include <stdio.h>
#ifndef __STB_INCLUDE_STB_TRUETYPE_H__ // Otherwise the application provides stb_truetype.
  #define STBTT_STATIC
  #define STB_TRUETYPE_IMPLEMENTATION
  #include "stb_truetype.h"
#endif

#define SLANT 0.207f // ~12deg, the usual oblique.

struct r_draw_face
{
    stbtt_fontinfo info;
    unsigned char* data;
};

struct r_draw_face_inst
{
    struct r_draw_face* face;
    float scale, embolden; // embolden in pixels.
    bool  slant;
};

struct font_state
{
    uint8_t*          bitmap; int cap_bitmap;
    r_draw_glyph_pos* pos;    int cap_pos;
};

static struct font_state* state(r_draw* c)
{
    if(!c->font_state) c->font_state = calloc(1, sizeof(struct font_state));
    return c->font_state;
}

void r_draw__face_free_state(r_draw* c)
{
    struct font_state* s = c->font_state;
    if(!s) return;
    free(s->bitmap); free(s->pos); free(s);
    c->font_state = NULL;
}

struct r_draw_face* r_draw__face_load(const char* path)
{
    FILE* fp = fopen(path, "rb");
    if(!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    struct r_draw_face* face = calloc(1, sizeof(*face));
    face->data = size > 0 ? malloc((size_t)size) : NULL;
    bool ok = face->data && fread(face->data, 1, (size_t)size, fp) == (size_t)size;
    fclose(fp);
    int offset = ok ? stbtt_GetFontOffsetForIndex(face->data, 0) : -1;
    if(offset < 0 || !stbtt_InitFont(&face->info, face->data, offset) || face->info.numGlyphs <= 0)
    {
        r_draw__face_free(face);
        return NULL;
    }
    return face;
}

void r_draw__face_free(struct r_draw_face* face)
{
    if(!face) return;
    free(face->data);
    free(face);
}

static uint32_t be16(const uint8_t* p) { return (uint32_t)p[0] << 8 | p[1]; }
static uint32_t be32(const uint8_t* p) { return be16(p) << 16 | be16(p + 2); }

int r_draw__face_weight(struct r_draw_face* face)
{
    const uint8_t* font = face->data + face->info.fontstart;
    for(uint32_t i = 0, n = be16(font + 4); i < n; i++)
    {
        const uint8_t* table = font + 12 + 16 * i;
        if(memcmp(table, "OS/2", 4) != 0) continue;
        int weight = (int)be16(face->data + be32(table + 8) + 4);
        return weight > 0 ? weight : 400;
    }
    return 400;
}

struct r_draw_face_inst* r_draw__face_inst(struct r_draw_face* face, int px, int weight, int synth_weight, bool italic)
{
    (void)weight;
    struct r_draw_face_inst* f = malloc(sizeof(*f));
    f->face     = face;
    f->scale    = stbtt_ScaleForMappingEmToPixels(&face->info, (float)px);
    f->embolden = synth_weight > 0 ? px * 0.03f * synth_weight / 300.0f : 0.0f;
    f->slant    = italic;
    return f;
}

void r_draw__face_inst_free(struct r_draw_face_inst* f)
{
    free(f);
}

void r_draw__face_inst_resize(struct r_draw_face_inst* f, int old_px, int px)
{
    f->scale = stbtt_ScaleForMappingEmToPixels(&f->face->info, (float)px);
    if(old_px > 0) f->embolden = f->embolden * px / old_px;
}

bool r_draw__face_glyph(struct r_draw_face_inst* f, uint32_t codepoint, uint32_t* gid)
{
    *gid = (uint32_t)stbtt_FindGlyphIndex(&f->face->info, (int)codepoint);
    return *gid != 0;
}

int r_draw__face_advance(struct r_draw_face_inst* f, uint32_t gid, bool* empty)
{
    int advance, lsb;
    stbtt_GetGlyphHMetrics(&f->face->info, (int)gid, &advance, &lsb);
    *empty = stbtt_IsGlyphEmpty(&f->face->info, (int)gid);
    return (int)((advance * f->scale + f->embolden) * 64.0f + 0.5f);
}

void r_draw__face_v_metrics(struct r_draw_face_inst* f, int* ascender, int* descender, int* line_gap)
{
    int asc, desc, gap;
    stbtt_GetFontVMetrics(&f->face->info, &asc, &desc, &gap);
    *ascender  = (int)(asc  * f->scale * 64.0f + 0.5f);
    *descender = (int)(desc * f->scale * 64.0f - 0.5f);
    *line_gap  = (int)(gap  * f->scale * 64.0f + 0.5f);
}

int r_draw__face_shape(r_draw* c, struct r_draw_face_inst* f, const char* text, int len, int start, int n, r_draw_glyph_pos** out)
{
    (void)len;
    struct font_state* s = state(c);
    s->pos = r_draw__grow(s->pos, &s->cap_pos, n, sizeof(r_draw_glyph_pos));
    int count = 0;
    for(const char* p = text + start, *end = p + n; p < end; )
    {
        uint32_t cluster = (uint32_t)(p - text), gid;
        r_draw__face_glyph(f, r_draw__decode_utf8(&p), &gid);
        if(count > 0) s->pos[count - 1].x_advance += (int)(stbtt_GetGlyphKernAdvance(&f->face->info, (int)s->pos[count - 1].glyph, (int)gid) * f->scale * 64.0f);
        bool empty;
        s->pos[count++] = (r_draw_glyph_pos){ gid, cluster, r_draw__face_advance(f, gid, &empty) };
    }
    *out = s->pos;
    return count;
}

bool r_draw__face_raster(r_draw* c, struct r_draw_face_inst* f, uint32_t gid, r_draw_glyph_bitmap* out)
{
    struct font_state* s = state(c);
    stbtt_vertex* v;
    int n = stbtt_GetGlyphShape(&f->face->info, (int)gid, &v);
    *out = (r_draw_glyph_bitmap){ .bpp = 1 };
    if(n <= 0) return true;

    float k = f->slant ? SLANT : 0.0f, x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    for(int i = 0; i < n; i++)
    {
        v[i].x  += (stbtt_vertex_type)(k * v[i].y);
        v[i].cx += (stbtt_vertex_type)(k * v[i].cy);
        v[i].cx1 += (stbtt_vertex_type)(k * v[i].cy1);
        x0 = MIN(x0, v[i].x); x1 = MAX(x1, v[i].x); y0 = MIN(y0, v[i].y); y1 = MAX(y1, v[i].y);
        if(v[i].type == STBTT_vcurve || v[i].type == STBTT_vcubic)
        { x0 = MIN(x0, v[i].cx); x1 = MAX(x1, v[i].cx); y0 = MIN(y0, v[i].cy); y1 = MAX(y1, v[i].cy); }
        if(v[i].type == STBTT_vcubic)
        { x0 = MIN(x0, v[i].cx1); x1 = MAX(x1, v[i].cx1); y0 = MIN(y0, v[i].cy1); y1 = MAX(y1, v[i].cy1); }
    }
    int ix0 = (int)floorf(x0 * f->scale), ix1 = (int)ceilf(x1 * f->scale + f->embolden);
    int iy0 = (int)floorf(-y1 * f->scale), iy1 = (int)ceilf(-y0 * f->scale);
    int w = ix1 - ix0, h = iy1 - iy0;
    if(w <= 0 || h <= 0) { stbtt_FreeShape(&f->face->info, v); return true; }

    int passes = f->embolden > 0 ? 2 : 1;
    s->bitmap = r_draw__grow(s->bitmap, &s->cap_bitmap, w * h * passes, 1);
    stbtt__bitmap bmp = { w, h, w, s->bitmap };
    stbtt_Rasterize(&bmp, 0.35f, v, n, f->scale, f->scale, 0.0f, 0.0f, ix0, iy0, 1, NULL);
    if(passes == 2)
    {
        stbtt__bitmap bold = { w, h, w, s->bitmap + w * h };
        stbtt_Rasterize(&bold, 0.35f, v, n, f->scale, f->scale, f->embolden, 0.0f, ix0, iy0, 1, NULL);
        for(int i = 0; i < w * h; i++) s->bitmap[i] = MAX(s->bitmap[i], bold.pixels[i]);
    }
    stbtt_FreeShape(&f->face->info, v);
    *out = (r_draw_glyph_bitmap){ s->bitmap, ix0, iy0, w, h, w, 1, false };
    return true;
}
#endif
