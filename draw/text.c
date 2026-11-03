#define _DEFAULT_SOURCE
#include "internal.h"

enum r_font_category
{
    R_FONT_CATEGORY_BASE, R_FONT_CATEGORY_BOLD, R_FONT_CATEGORY_MONO,
    R_FONT_CATEGORY_CJK, R_FONT_CATEGORY_ARABIC, R_FONT_CATEGORY_HEBREW,
    R_FONT_CATEGORY_THAI, R_FONT_CATEGORY_INDIC, R_FONT_CATEGORY_EMOJI,
    R_FONT_CATEGORY_SYMBOLS, R_FONT_CATEGORY_KOREAN, R_FONT_CATEGORY_MAX
};

#if defined(_WIN32)
  typedef struct _WIN32_FIND_DATAA {
    uint32_t dwFileAttributes;
    uint32_t ftCreationTime[2], ftLastAccessTime[2], ftLastWriteTime[2];
    uint32_t nFileSizeHigh, nFileSizeLow;
    uint32_t dwReserved0, dwReserved1;
    char     cFileName[260];
    char     cAlternateFileName[14];
  } WIN32_FIND_DATAA;

  #define INVALID_HANDLE_VALUE     ((void*)(intptr_t)-1)
  #define FILE_ATTRIBUTE_DIRECTORY 0x00000010UL

  void* __stdcall FindFirstFileA(const char* lpFileName, WIN32_FIND_DATAA* lpFindFileData);
  int   __stdcall FindNextFileA (void* hFindFile, WIN32_FIND_DATAA* lpFindFileData);
  int   __stdcall FindClose     (void* hFindFile);
#elif !defined(RCUT_NO_SYSTEM_FONTS)
  #include <dirent.h>
  #include <sys/stat.h>
#endif

struct font_search
{
    const char* const* names[R_FONT_CATEGORY_MAX];
    int   best_variant[R_FONT_CATEGORY_MAX];
    char  path[R_FONT_CATEGORY_MAX][512];
    int   depth;
};

static bool ascii_ieq(const char* a, const char* b, int n)
{
    for(int i = 0; i < n; i++)
    {
        char x = a[i] >= 'A' && a[i] <= 'Z' ? a[i] + 32 : a[i];
        char y = b[i] >= 'A' && b[i] <= 'Z' ? b[i] + 32 : b[i];
        if(x != y) return false;
        if(!x) return true;
    }
    return true;
}

static void scan_font_dir(struct font_search* search, const char* dir);

static void visit_font_file(struct font_search* search, const char* dir, const char* fname, bool is_dir)
{
    if(fname[0] == '.') return;
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, fname);
    if(is_dir)
    {
        if(search->depth < 4)
        {
            search->depth++;
            scan_font_dir(search, path);
            search->depth--;
        }
        return;
    }

    const char* const extensions[] = {".ttf", ".ttc", ".otf", NULL};
    int fname_len = (int)strlen(fname);
    bool is_font = false;
    for(int j = 0; extensions[j] != NULL; j++)
        is_font |= fname_len > 4 && ascii_ieq(fname + fname_len - 4, extensions[j], 4);
    if(!is_font) return;

    for(int c = 0; c < R_FONT_CATEGORY_MAX; c++)
    for(int i = 0; i < search->best_variant[c] && search->names[c][i] != NULL; i++)
    {
        int name_len = (int)strlen(search->names[c][i]);
        if(fname_len == name_len + 4 && ascii_ieq(fname, search->names[c][i], name_len))
        {
            search->best_variant[c] = i;
            snprintf(search->path[c], sizeof(search->path[c]), "%s", path);
            break;
        }
    }
}

static void scan_font_dir(struct font_search* search, const char* dir)
{
#if defined(RCUT_NO_SYSTEM_FONTS)
    (void)search; (void)dir;
#elif defined(_WIN32)
    char pattern[512];
    snprintf(pattern, sizeof(pattern), "%s\\*", dir);
    WIN32_FIND_DATAA fd;
    void* h = FindFirstFileA(pattern, &fd);
    if(h == INVALID_HANDLE_VALUE) return;
    do visit_font_file(search, dir, fd.cFileName, fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    while(FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR* d = opendir(dir);
    if(!d) return;
    for(struct dirent* e; (e = readdir(d)) != NULL; )
    {
        bool is_dir = e->d_type == DT_DIR;
        if(e->d_type == DT_UNKNOWN || e->d_type == DT_LNK)
        {
            char path[512]; struct stat st;
            snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
            is_dir = stat(path, &st) == 0 && S_ISDIR(st.st_mode);
        }
        visit_font_file(search, dir, e->d_name, is_dir);
    }
    closedir(d);
#endif
}

void r_draw_find_system_fonts(r_draw* frontend, bool include_base_fonts)
{
    static const char* const base_font [] = {"segoeui", "inter-regular", "roboto-regular", "notosans-regular", "opensans-regular", "cantarell-regular", "arial", "liberationsans-regular", "dejavusans", NULL};
    static const char* const bold      [] = {"segoeuib", "inter-semibold", "roboto-medium", "notosans-semibold", "opensans-semibold", "cantarell-bold", "arialbd", "liberationsans-bold", "dejavusans-bold", NULL};
    static const char* const mono      [] = {"consola", "jetbrainsmono-regular", "robotomono-regular", "notosansmono-regular", "liberationmono-regular", "dejavusansmono", "cour", NULL};

    static const char* const fb_cjk    [] = {"msyh", "yugothm", "yugothr", "malgun", "msgothic", "simsun", "pingfang", "hiraginosansgb", "applesdgothicneo", "notosanscjk-regular", "notosanscjksc-regular", "notosansjp-regular", "wqy-microhei", "droidsansfallbackfull", NULL};
    static const char* const fb_arabic [] = {"tahoma",   "arabtype", "geezapro", "notosansarabic-regular", "amiri-regular", NULL };
    static const char* const fb_hebrew [] = {"arialhb",  "david", "notosanshebrew-regular", NULL };
    static const char* const fb_thai   [] = {"leelawui", "tahoma", "thonburi", "notosansthai-regular", NULL };
    static const char* const fb_indic  [] = {"nirmala",  "kohinoordevanagari", "devanagarisangammn", "notosansdevanagari-regular", "lohit-devanagari", NULL };
    static const char* const fb_emoji  [] = {"seguiemj", "apple color emoji", "notocoloremoji", NULL };
    static const char* const fb_symbols[] = {"seguisym", "notosanssymbols-regular", "notosanssymbols2-regular", "symbola", "dejavusans", NULL };
    // After CJK: Fallbacks are tried in order, so Hangul missing from the chosen CJK font lands here without Hanja switching fonts.
    static const char* const fb_korean [] = {"malgun", "applesdgothicneo", "notosanskr-regular", "notosanscjkkr-regular", "nanumgothic", "unfonts-core", NULL };

    static const struct { int slot; uint32_t ranges[8][2]; } categories[R_FONT_CATEGORY_MAX] =
    {
        { 0 }, { 1 }, { 2 },
        { 32, { {0x3000, 0x303F}, {0x3040, 0x30FF}, {0x3100, 0x318F}, {0x3400, 0x4DBF}, {0x4E00, 0x9FFF}, {0xAC00, 0xD7AF}, {0xF900, 0xFAFF}, {0xFF00, 0xFFEF} } },
        { 33, { {0x0600, 0x06FF}, {0x0750, 0x077F}, {0x08A0, 0x08FF}, {0xFB50, 0xFDFF}, {0xFE70, 0xFEFF} } },
        { 34, { {0x0590, 0x05FF}, {0xFB1D, 0xFB4F} } },
        { 35, { {0x0E00, 0x0E7F} } },
        { 36, { {0x0900, 0x097F}, {0x0980, 0x09FF}, {0x0A00, 0x0A7F}, {0x0A80, 0x0AFF}, {0x0B00, 0x0B7F}, {0x0B80, 0x0BFF}, {0x0C00, 0x0C7F}, {0x0C80, 0x0CFF} } },
        { 37 }, { 38 },
        { 39, { {0x1100, 0x11FF}, {0x3130, 0x318F}, {0xA960, 0xA97F}, {0xAC00, 0xD7AF}, {0xD7B0, 0xD7FF} } },
    };

    struct font_search search = { .names = { base_font, bold, mono, fb_cjk, fb_arabic, fb_hebrew, fb_thai, fb_indic, fb_emoji, fb_symbols, fb_korean } };
    for(int i = 0; i < R_FONT_CATEGORY_MAX; i++)
        search.best_variant[i] = 9999;
    if(!include_base_fonts) search.best_variant[R_FONT_CATEGORY_BASE] = search.best_variant[R_FONT_CATEGORY_BOLD] = search.best_variant[R_FONT_CATEGORY_MONO] = 0;

    char buf[1024];
    arena_t arena = {.start=(uint8_t*)buf, .cur=(uint8_t*)buf, .end=(uint8_t*)&buf[sizeof(buf)]};
    const char* folders[8]; int num_folders = 0;
    #define add(STR, ...) folders[num_folders++] = arena_push(&arena, snprintf((char*)arena.cur, arena.end - arena.cur, STR, __VA_ARGS__) + 2);

#if defined(_WIN32)
    const char* wd = getenv("WINDIR"); if(!wd) wd = "C:\\Windows";
    add("%s\\Fonts", wd);
    const char* appdata = getenv("LOCALAPPDATA");
    if(appdata) add("%s\\Microsoft\\Windows\\Fonts", appdata);
#elif defined(__APPLE__)
    folders[num_folders++] = "/System/Library/Fonts";
    folders[num_folders++] = "/System/Library/Fonts/Supplemental";
    folders[num_folders++] = "/Library/Fonts";
    const char* home = getenv("HOME");
    if(home) add("%s/Library/Fonts", home);
#elif defined(__ANDROID__)
    folders[num_folders++] = "/system/fonts";
#else
    folders[num_folders++] = "/usr/share/fonts";
    folders[num_folders++] = "/usr/local/share/fonts";
    const char* home = getenv("HOME");
    if(home)
    {
        add("%s/.local/share/fonts", home);
        add("%s/.fonts", home);
    }
#endif
    #undef add

    for(int i = 0; i < num_folders; i++)
        scan_font_dir(&search, folders[i]);

    for(int list = 0; list < R_FONT_CATEGORY_MAX; list++)
    {
        if(search.best_variant[list] == 9999) continue;
        if(list == R_FONT_CATEGORY_BOLD && search.best_variant[R_FONT_CATEGORY_BOLD] != search.best_variant[R_FONT_CATEGORY_BASE]) continue;
        int index = categories[list].slot;
        #ifdef VERBOSE
        printf("Font category %d = %s\n", list, search.path[list]);
        #endif
        if(!r_draw_font_register(frontend, index, search.path[list], list >= R_FONT_CATEGORY_CJK)) continue;
        for(int i = 0; i < 8; i++)
        {
            frontend->base_fonts[index].codepoint_ranges[i].min = categories[list].ranges[i][0];
            frontend->base_fonts[index].codepoint_ranges[i].max = categories[list].ranges[i][1];
        }
        if(list >= R_FONT_CATEGORY_CJK)
            frontend->font_system_fallbacks[frontend->font_system_num_fallbacks++] = index;
    }
    if(!include_base_fonts) return;
    for(int i = 0; i < 3; i++)
        if(frontend->base_fonts[i].path) r_draw_font_set_fallbacks(frontend, i, NULL, 0, true);
    if(frontend->base_fonts[1].path) frontend->base_fonts[0].bold_redirect = 1;
}

static uint64_t font_hash(uint64_t base, uint64_t size, uint64_t flags)
{
    uint64_t h = base * 0x9E3779B97F4A7C15ULL;
    h ^= size  + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
    h ^= flags + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
    h ^= h >> 33; h *= 0xFF51AFD7ED558CCDULL; h ^= h >> 33;
    return h;
}

bool r_draw_font_register(r_draw* c, int slot, const char* font_path, bool lazy)
{
    struct r_draw_face* face = NULL;
    if(!lazy && !(face = __draw_face_load(font_path))) return false;

    if(lazy)
    {
        FILE* f = fopen(font_path, "rb");
        if(!f) return false;
        fclose(f);
    }

    struct r_draw_base_font* base_font = &c->base_fonts[slot];
    base_font->path = malloc(strlen(font_path) + 1);
    strcpy(base_font->path, font_path);
    base_font->face = face;
    base_font->weight = face ? (int16_t)__draw_face_weight(face) : 400;
    base_font->italic_redirect = -1; // No dedicated italic face: synthesize a slant.
    base_font->bold_redirect   = -1; // No dedicated bold face:  synthesize weight.
    for(int i = 0; i < 16; i++)
        base_font->fallbacks[i] = -1;
    base_font->lazy_load = lazy;
    return true;
}

static bool ensure_font_is_loaded(r_draw* c, int slot)
{
    if(c->base_fonts[slot].lazy_load && c->base_fonts[slot].face == NULL)
    {
        struct r_draw_base_font* base_font = &c->base_fonts[slot];
        base_font->face = __draw_face_load(base_font->path);
        if(base_font->face) base_font->weight = (int16_t)__draw_face_weight(base_font->face);
    }
    return c->base_fonts[slot].face != NULL;
}

void r_draw_font_set_fallbacks(r_draw* c, int base_font, int* fallbacks, int num_fallbacks, bool include_system_fallbacks)
{
    struct r_draw_base_font* base = &c->base_fonts[base_font];

    int cap = 16;
    if(num_fallbacks > cap) num_fallbacks = cap;

    int i = 0;
    if(fallbacks !=NULL)
    for(; i < num_fallbacks; i++)
        base->fallbacks[i] = fallbacks[i];

    if(include_system_fallbacks)
    for(int j = 0; i < cap && j < c->font_system_num_fallbacks; i++, j++)
        base->fallbacks[i] = c->font_system_fallbacks[j];

    for(; i < cap; i++)
        base->fallbacks[i] = -1;
}

r_draw_font* r_draw_get_font(r_draw* c, int base_font, int size, uint32_t flags)
{
    if(size <= 0) return NULL;

    ensure_font_is_loaded(c, base_font);

    flags &= (R_FONT_WEIGHT_MASK | R_FONT_ITALIC | R_FONT_SHAPED);
    bool want_italic = flags & R_FONT_ITALIC;
    int  want_weight = (flags & R_FONT_WEIGHT_MASK) * 100;

    // Prefer a dedicated face over synthesis when the base font declares one.
    int      resolved_base = base_font;
    uint32_t resolved_flags = flags;
    if(want_italic && c->base_fonts[base_font].italic_redirect >= 0)
    {
        resolved_base   = c->base_fonts[base_font].italic_redirect;
        resolved_flags &= ~(uint32_t)R_FONT_ITALIC;
    }
    int bold = c->base_fonts[resolved_base].bold_redirect;
    bool bold_native = want_weight && bold >= 0 && ensure_font_is_loaded(c, bold) && want_weight >= c->base_fonts[bold].weight;
    if(bold_native) resolved_base = bold;

    uint64_t h = font_hash((uint64_t)resolved_base, (uint64_t)size, resolved_flags);
    int start = (int)(h & (RCUT_DRAW_FONT_HASHMAP_SIZE) - 1);

    int free_slot = -1;
    for(int probe = 0; probe < RCUT_DRAW_FONT_HASHMAP_SIZE; probe++)
    {
        int slot = (start + probe) & (RCUT_DRAW_FONT_HASHMAP_SIZE - 1);
        r_draw_font* f = &c->fonts[slot];
        if(!f->font) { free_slot = slot; break; } // Empty: end of probe chain.
        if(f->base_font_idx == resolved_base && f->size == size && f->resolved_flags == resolved_flags)
            return f; // Cache hit.
    }
    if(free_slot < 0) return NULL; // Cache full.

    if(!ensure_font_is_loaded(c, resolved_base)) return NULL;
    int px = (int)(size * c->scale + 0.5f);
    int synth_weight = want_weight && !bold_native ? want_weight - c->base_fonts[resolved_base].weight : 0;
    struct r_draw_face_inst* font = __draw_face_inst(c->base_fonts[resolved_base].face, px, want_weight, synth_weight, resolved_flags & R_FONT_ITALIC);
    if(!font) return NULL;

    r_draw_font* f = &c->fonts[free_slot];
    f->base_font_idx = resolved_base;
    f->size          = size;
    f->scale         = c->scale;
    f->flags         = flags;
    f->resolved_flags = resolved_flags;
    f->font          = font;
    return f;
}

void __draw_rescale_fonts(r_draw* c)
{
    for(int i = 0; i < RCUT_DRAW_FONT_HASHMAP_SIZE; i++)
    {
        r_draw_font* f = &c->fonts[i];
        if(!f->font) continue;
        int old_px = (int)(f->size * f->scale + 0.5f), px = (int)(f->size * c->scale + 0.5f);
        __draw_face_inst_resize(f->font, old_px, px);
        f->scale = c->scale;
    }
}

static inline uint32_t hash32(uint32_t a, uint32_t b)
{
    uint32_t hash = 2166136261u;
    hash ^= a; hash *= 16777619u;
    hash ^= b; hash *= 16777619u;
    hash ^= hash >> 16; hash *= 0x85ebca6bu;
    hash ^= hash >> 13; hash *= 0xc2b2ae35u;
    hash ^= hash >> 16;
    return hash;
}

static inline int get_font_id(r_draw* c, r_draw_font* font)
{
    return (int)(font - c->fonts);
}
static int glyph_find(r_draw* c, int font_id, uint16_t glyph_id)
{
    int start = (int)(hash32((uint32_t)font_id, glyph_id) % (uint32_t)RCUT_DRAW_MAX_GLYPHS);

    int hash = start;
    while(c->glyphs[hash].font_id != 0 || c->glyphs[hash].glyph_id != 0)
    {
        if(c->glyphs[hash].font_id == font_id && c->glyphs[hash].glyph_id == glyph_id)
            return hash;
        hash = (hash + 1) % RCUT_DRAW_MAX_GLYPHS;
        if(hash == start) break;
    }
    return -1;
}

static int find_glyph_id(r_draw* c, r_draw_font* font, uint16_t glyph_id)
{
    if(!font || !font->font) return -1;
    return glyph_find(c, get_font_id(c, font), glyph_id);
}

static int glyph_slot(r_draw* c, int font_id, uint16_t glyph_id)
{
    if(c->num_glyphs >= RCUT_DRAW_GLYPH_LOAD_LIMIT) { c->glyphs_full = true; return -1; }
    int hash = (int)(hash32((uint32_t)font_id, glyph_id) % (uint32_t)RCUT_DRAW_MAX_GLYPHS);
    while(c->glyphs[hash].font_id != 0 || c->glyphs[hash].glyph_id != 0)
    {
        assert(!(c->glyphs[hash].font_id == font_id && c->glyphs[hash].glyph_id == glyph_id));
        hash = (hash + 1) % RCUT_DRAW_MAX_GLYPHS;
    }
    c->num_glyphs++;
    return hash;
}

static int icon_glyph(r_draw* c, int icon, int size)
{
    if(icon <= 0 || icon >= COUNTOF(c->icons) || !c->icons[icon].ops || size <= 0 || size > 256) return -1;
    int font_id = -size;
    int gi = glyph_find(c, font_id, (uint16_t)icon);
    if(gi >= 0) return gi;
    gi = glyph_slot(c, font_id, (uint16_t)icon);
    if(gi < 0) return -1;
    c->glyphs[gi] = (struct r_draw_glyph){ .font_id = font_id, .glyph_id = (uint16_t)icon, .x_advance = size << 6 };
    return gi;
}

static int cache_glyph(r_draw* c, int font_id, uint16_t glyph_id)
{
    struct r_draw_face_inst* f = c->fonts[font_id].font;
    if(!f) return -1;
    int hash = glyph_slot(c, font_id, glyph_id);
    if(hash < 0) return -1;

    struct r_draw_glyph* g = &c->glyphs[hash];
    *g = (struct r_draw_glyph){0};
    g->font_id    = font_id;
    g->glyph_id   = glyph_id;
    g->x_advance  = __draw_face_advance(f, glyph_id, &g->is_empty);
    return hash;
}

static uint8_t* glyph_effect(const uint8_t* src, int stride, int bpp, int w, int h, int dilate, int blur, int margin)
{
    int ow = w + 2 * margin, oh = h + 2 * margin;
    uint8_t* out = calloc((size_t)ow * oh, 1);
    for(int y = 0; y < oh; y++)
    for(int x = 0; x < ow; x++)
    {
        int v = 0;
        for(int dy = -dilate; dy <= dilate; dy++)
        for(int dx = -dilate; dx <= dilate; dx++)
        {
            int sx = x - margin + dx, sy = y - margin + dy;
            if(sx >= 0 && sy >= 0 && sx < w && sy < h && dx * dx + dy * dy <= dilate * dilate + dilate)
                v = MAX(v, src[sy * stride + sx * bpp + bpp - 1]);
        }
        out[y * ow + x] = (uint8_t)v;
    }
    uint8_t* line = malloc(MAX(ow, oh));
    for(int pass = 0; blur && pass < 4; pass++)
    {
        bool rows = !(pass & 1);
        int n = rows ? ow : oh, lines = rows ? oh : ow, step = rows ? 1 : ow;
        for(int j = 0; j < lines; j++)
        {
            uint8_t* p = out + (rows ? j * ow : j);
            for(int i = 0; i < n; i++) line[i] = p[i * step];
            for(int i = 0; i < n; i++)
            {
                int sum = 0;
                for(int k = MAX(i - blur, 0); k <= MIN(i + blur, n - 1); k++) sum += line[k];
                p[i * step] = (uint8_t)(sum / (2 * blur + 1));
            }
        }
    }
    free(line);
    return out;
}

static uint64_t cache_font_glyph(r_draw* c, struct r_draw_glyph* g)
{
    int font_id = g->font_id % RCUT_DRAW_FONT_HASHMAP_SIZE, effect = g->font_id / RCUT_DRAW_FONT_HASHMAP_SIZE;
    r_draw_glyph_bitmap bm;
    if(!__draw_face_raster(c, c->fonts[font_id].font, g->glyph_id, &bm)) { g->is_empty = true; return 0; }

    int dilate = effect & 15, blur = effect >> 4, margin = dilate + 2 * blur;
    g->x_origin   = bm.x - margin;
    g->y_origin   = bm.y - margin;
    g->w          = bm.w + 2 * margin;
    g->h          = bm.h + 2 * margin;
    g->is_colored = !effect && bm.colored;
    g->is_empty   = bm.w == 0 || bm.h == 0;

    uint64_t handle = 0;
    if(g->is_empty) {}
    else if(g->is_colored) handle = c->backend.cache_argb(c->backend.userptr, g->w, g->h, (const uint32_t*)bm.pixels, bm.stride, false);
    else
    {
        uint8_t* coverage = glyph_effect(bm.pixels, bm.stride, bm.bpp, bm.w, bm.h, dilate, blur, margin);
        handle = c->backend.cache_monochrome(c->backend.userptr, g->w, g->h, coverage, g->w, false);
        free(coverage);
    }
    return handle;
}

bool __draw_glyph_pixels(r_draw* c, int glyph_idx)
{
    struct r_draw_glyph* g = &c->glyphs[glyph_idx];
    if(g->is_empty) return false;
    if(g->handle && c->backend.is_cached(c->backend.userptr, g->handle)) return true;
    g->handle = 0;
    if(c->cache_full) return false;
    if(g->font_id < 0)
    {
        int size = -g->font_id;
        g->handle = __draw_cache_icon(c, g->glyph_id, size, &g->is_colored);
        g->w = g->h = size;
    }
    else g->handle = cache_font_glyph(c, g);
    if(!g->handle && !g->is_empty) c->cache_full = true;
    return g->handle != 0;
}

static int glyph_idx_cached(r_draw* c, r_draw_font* font, uint16_t glyph_id)
{
    int idx = find_glyph_id(c, font, glyph_id);
    if(idx < 0) idx = cache_glyph(c, get_font_id(c, font), glyph_id);
    return idx;
}

static int get_codepoint_glyph(r_draw* c, r_draw_font* font, uint32_t codepoint)
{
    if(!font || !font->font) return -1;

    int font_id = get_font_id(c, font);
    uint32_t start = hash32((uint32_t)font_id, codepoint) % (uint32_t)RCUT_DRAW_MAX_GLYPHS;

    uint32_t slot = start;
    do
    {
        struct r_draw_glyph_mapping* m = &c->glyph_mapping[slot];
        if(m->codepoint == 0) // Empty: resolve and cache.
        {
            if(c->num_glyph_mappings >= RCUT_DRAW_GLYPH_LOAD_LIMIT) { c->glyphs_full = true; return -1; }
            c->num_glyph_mappings++;
            uint32_t gid, glyph_idx = -1;
            if(__draw_face_glyph(font->font, codepoint, &gid) && gid != 0)
            {
                int idx = glyph_idx_cached(c, font, (uint16_t)gid);
                if(idx >= 0) glyph_idx = (uint32_t)idx;
            }
            m->font_id   = font_id;
            m->codepoint = codepoint;
            m->glyph_idx = glyph_idx; // -1 means: try the fallbacks.
            return glyph_idx;
        }
        if(m->font_id == font_id && m->codepoint == codepoint)
            return m->glyph_idx;
        slot = (slot + 1) % RCUT_DRAW_MAX_GLYPHS;
    } while(slot != start);

    return -1; // Full.
}

static bool is_emoji(uint32_t cp) { return cp >= 0x1F300 && cp <= 0x1FAFF; }

static int resolve_codepoint_glyph(r_draw* c, r_draw_font* font, uint32_t cp)
{
    uint32_t gi = is_emoji(cp) ? -1 : get_codepoint_glyph(c, font, cp);
    if(gi != -1) return (int)gi;

    struct r_draw_base_font* base_font = &c->base_fonts[font->base_font_idx];
    for(int i = 0; i < (int)(sizeof(base_font->fallbacks) / sizeof(base_font->fallbacks[0])); i++)
    {
        if(base_font->fallbacks[i] < 0) break;
        r_draw_font* ff = r_draw_get_font(c, base_font->fallbacks[i], font->size, font->flags);
        if(!ff) continue;
        gi = get_codepoint_glyph(c, ff, cp);
        if(gi != -1) return (int)gi;
    }
    return is_emoji(cp) ? (int)get_codepoint_glyph(c, font, cp) : -1;
}

typedef struct run_glyph { int32_t glyph, cluster, x_advance; int16_t x_offset, y_offset; } run_glyph;
typedef struct laid_glyph { int32_t glyph, x; int16_t y, line; } laid_glyph;

typedef struct text_layout
{
    r_draw_font* font;
    uint32_t     generation;
    int          wrap_width, len;
    const char*  text;
    int          num_lines, num_glyphs;
    int32_t*     line_w;
    laid_glyph*  glyphs;
} text_layout;

struct r_draw_text
{
    uint32_t     generation;
    run_glyph*   run;    int cap_run;
    laid_glyph*  laid;   int cap_laid;
    int32_t*     line_w; int cap_lines;
};

static struct r_draw_text* scratch(r_draw* c)
{
    if(!c->text) c->text = calloc(1, sizeof(struct r_draw_text));
    return c->text;
}

void __draw_clear_glyph_cache(r_draw* c)
{
    for(int i = 0; i < RCUT_DRAW_MAX_GLYPHS; i++)
        __draw_invalidate(c, &c->glyphs[i].handle);
    c->glyphs_full = false;
    c->num_glyphs = c->num_glyph_mappings = 0;
    memset(c->glyphs, 0, sizeof(c->glyphs));
    memset(c->glyph_mapping, 0, sizeof(c->glyph_mapping));
    if(c->text) c->text->generation++;
}

void __draw_free_text(r_draw* c)
{
    if(!c->text) return;
    free(c->text->run); free(c->text->laid); free(c->text->line_w);
    free(c->text);
    c->text = NULL;
}

static r_draw_font* default_font(r_draw* c, r_draw_font* font)
{
    if(font == NULL) font = r_draw_get_font(c, 0, 14, 0);
    return font && font->font ? font : NULL;
}

static int shape_run(r_draw* c, r_draw_font* font, const char* text, int len)
{
    struct r_draw_text* t = scratch(c);
    const char* end = text + len;
    int n = 0;
    if(!(font->flags & R_FONT_SHAPED))
    {
        t->run = __draw_grow(t->run, &t->cap_run, len, sizeof(run_glyph));
        for(const char* p = text; p < end; )
        {
            int cluster = (int)(p - text);
            int gi = resolve_codepoint_glyph(c, font, __draw_decode_utf8(&p));
            t->run[n++] = (run_glyph){ gi, cluster, gi >= 0 ? c->glyphs[gi].x_advance : 0 };
        }
        return n;
    }
    for(const char* p = text; p < end; )
    {
        const char* start = p;
        r_draw_font* run_font = NULL;
        while(p < end)
        {
            const char* q = p;
            int gi = resolve_codepoint_glyph(c, font, __draw_decode_utf8(&q));
            r_draw_font* f = gi >= 0 ? &c->fonts[c->glyphs[gi].font_id] : run_font ? run_font : font;
            if(run_font && f != run_font) break;
            run_font = f;
            p = q;
        }
        r_draw_glyph_pos* pos;
        int count = __draw_face_shape(c, run_font->font, text, len, (int)(start - text), (int)(p - start), &pos);
        t->run = __draw_grow(t->run, &t->cap_run, n + count, sizeof(run_glyph));
        for(int i = 0; i < count; i++)
        {
            int gi = pos[i].glyph ? glyph_idx_cached(c, run_font, (uint16_t)pos[i].glyph) : -1;
            t->run[n++] = (run_glyph){ gi, (int)pos[i].cluster, pos[i].x_advance, (int16_t)pos[i].x_offset, (int16_t)pos[i].y_offset };
        }
    }
    return n;
}

static int run_width(r_draw* c, r_draw_font* font, const char* text, int len)
{
    int n = len > 0 ? shape_run(c, font, text, len) : 0, w = 0;
    for(int i = 0; i < n; i++) w += c->text->run[i].x_advance;
    return w;
}

static const char* next_line(r_draw* c, r_draw_font* font, const char* line, const char* end, int wrap_width, bool force, const char** next, int* width)
{
    const char* p = line;
    int w = 0;
    // Widths are measured to the nearest pixel, so text may overhang its measured width by up to half a pixel (+1 device pixel of truncation).
    int limit = (wrap_width << 6) + (int)(font->scale * 32) + 64;
    if(wrap_width <= 0)
    {
        while(p < end && *p != '\n') p++;
        w = run_width(c, font, line, (int)(p - line));
    }
    else while(p < end && *p != '\n')
    {
        const char* q = p;
        while(q < end && *q != ' ' && *q != '\n') q++;
        int word_w = run_width(c, font, p, (int)(q - p));
        if(w + word_w > limit)
        {
            if(p > line || !force) { *next = p; return p; }
            int n = shape_run(c, font, p, (int)(q - p)), cut = (int)(q - p);
            for(int i = 0; i < n; i++)
            {
                run_glyph* g = &c->text->run[i];
                if(w + g->x_advance > limit && g->cluster > 0) { cut = g->cluster; break; }
                w += g->x_advance;
            }
            *next = p + cut; *width = w;
            return p + cut;
        }
        w += word_w;
        *width = w;
        const char* s = q;
        while(s < end && *s == ' ') s++;
        w += run_width(c, font, q, (int)(s - q));
        p = s;
    }
    *next = p < end && *p == '\n' ? p + 1 : p;
    *width = w;
    return p;
}

static int line_height_px(r_draw_font* font)
{
    int asc, desc, gap;
    __draw_face_v_metrics(font->font, &asc, &desc, &gap);
    int line_height = (asc - desc + gap + 63) / 64;
    return line_height > 0 ? line_height : (int)(font->size * font->scale);
}

int r_draw_font_line_height(r_draw_font* font)
{
    return (int)ceilf(line_height_px(font) / font->scale);
}

rect r_draw_measure_text_wrapped(r_draw* c, r_draw_font* font, const char* text, int len, int wrap_width)
{
    font = default_font(c, font);
    if(!font || !text) return (rect){0};
    if(len < 0) len = (int)strlen(text);
    const char* end = text + len;
    int max_w = 0, lines = 0;
    wrap_width = (int)(wrap_width * font->scale);
    for(const char* line = text; ; lines++)
    {
        const char* next; int w = 0;
        const char* eol = next_line(c, font, line, end, wrap_width, true, &next, &w);
        max_w = MAX(max_w, w);
        if(eol >= end) break;
        line = next;
    }
    return (rect){.w = (int)roundf(max_w / 64.0f / font->scale), .h = (int)ceilf((lines + 1) * line_height_px(font) / font->scale)};
}

rect r_draw_measure_text(r_draw* c, r_draw_font* font, const char* text, int len)
{
    return r_draw_measure_text_wrapped(c, font, text, len, 0);
}

int r_draw_break_text(r_draw* c, r_draw_font* font, const char* text, int len, int width, bool force, int* w)
{
    font = default_font(c, font);
    *w = 0;
    if(!font || !text) return 0;
    if(len < 0) len = (int)strlen(text);
    const char* next; int line_w = 0;
    const char* eol = next_line(c, font, text, text + len, MAX((int)(width * font->scale), 1), force, &next, &line_w);
    *w = (int)roundf(line_w / 64.0f / font->scale);
    return (int)(eol - text);
}

static text_layout layout_text(r_draw* c, r_draw_font* font, const char* text, int len, int wrap_width)
{
    struct r_draw_text* t = scratch(c);
    const char* end = text + len;
    int n = 0, lines = 0;
    for(const char* line = text; ; )
    {
        const char* next; int w = 0;
        const char* eol = next_line(c, font, line, end, wrap_width, true, &next, &w);
        t->line_w = __draw_grow(t->line_w, &t->cap_lines, lines + 1, sizeof(int32_t));
        t->line_w[lines] = w;
        int count = shape_run(c, font, line, (int)(eol - line)), x = 0;
        t->laid = __draw_grow(t->laid, &t->cap_laid, n + count, sizeof(laid_glyph));
        for(int i = 0; i < count; i++)
        {
            run_glyph* g = &t->run[i];
            t->laid[n++] = (laid_glyph){ g->glyph, x + g->x_offset, g->y_offset, (int16_t)lines };
            x += g->x_advance;
        }
        lines++;
        if(eol >= end) break;
        line = next;
    }
    return (text_layout){ font, t->generation, wrap_width, len, text, lines, n, t->line_w, t->laid };
}

static bool layout_matches(r_draw* c, const text_layout* t, r_draw_font* font, const char* text, int len, int wrap_width)
{
    return t && t->font == font && t->generation == c->text->generation && t->wrap_width == wrap_width && t->len == len && !memcmp(t->text, text, len);
}

static text_layout drawable_layout(r_draw* c, r_drawable* d, r_draw_font* font, int wrap_width)
{
    int len = (int)strlen(d->text);
    scratch(c);
    if(layout_matches(c, d->layout_cache, font, d->text, len, wrap_width)) return *(text_layout*)d->layout_cache;
    r_ui_list* l = c->pass_list;
    int index = l ? (int)(c->pass_item - l->draw_items) : -1;
    if(index < 0 || index >= l->count_draw_items || d->layout_cache) return layout_text(c, font, d->text, len, wrap_width);

    text_layout* found = NULL;
    int n = l->count_draw_items_last_frame;
    for(int k = 0; k < n && !found; k++)
    {
        r_drawable* o = &l->draw_items_last_frame[(MIN(index, n - 1) + k) % n];
        if(o->r.id == d->r.id && layout_matches(c, o->layout_cache, font, d->text, len, wrap_width)) found = o->layout_cache;
    }
    text_layout t = found ? *found : layout_text(c, font, d->text, len, wrap_width);

    int glyphs = t.num_glyphs * (int)sizeof(laid_glyph), lines = t.num_lines * (int)sizeof(int32_t);
    text_layout* kept = r_storage_alloc(l, (int)sizeof(text_layout) + glyphs + lines + len + 1);
    *kept = t;
    kept->glyphs = memcpy(kept + 1, t.glyphs, glyphs);
    kept->line_w = memcpy((char*)kept->glyphs + glyphs, t.line_w, lines);
    kept->text   = memcpy((char*)kept->line_w + lines, d->text, len + 1);
    c->pass_item->layout_cache = kept;
    return t;
}

static int effect_glyph(r_draw* c, int gi, int dilate, int blur)
{
    int effect = dilate | blur << 4;
    if(gi < 0 || !effect) return gi;
    struct r_draw_glyph* g = &c->glyphs[gi];
    int font_id = g->font_id + RCUT_DRAW_FONT_HASHMAP_SIZE * effect;
    int idx = glyph_find(c, font_id, g->glyph_id);
    if(idx >= 0) return idx;
    idx = glyph_slot(c, font_id, g->glyph_id);
    if(idx >= 0) c->glyphs[idx] = (struct r_draw_glyph){ .font_id = font_id, .glyph_id = g->glyph_id, .x_advance = g->x_advance, .is_empty = g->is_empty };
    return idx;
}

static int effect_px(float v, float scale){ return v > 0 ? MIN(MAX((int)(v * scale + 0.5f), 1), 15) : 0; }

static void render_layout(r_draw* c, const text_layout* t, uint32_t color, const r_draw_style_rule* fx, rect bounds, r_alignment align, int scroll_x, int scroll_y, rect clip)
{
    float s = c->scale;
    bounds = __draw_to_pixels(bounds, s);
    clip = rect_intersect(__draw_to_pixels(clip, s), c->clip_px);
    scroll_x = (int)(scroll_x * s); scroll_y = (int)(scroll_y * s);
    int asc, desc, gap;
    __draw_face_v_metrics(t->font->font, &asc, &desc, &gap);
    int ascender    = (asc + 32) / 64;
    int text_height = (asc - desc + 32) / 64;
    int line_height = line_height_px(t->font);
    int block_h = (t->num_lines - 1) * line_height + text_height;
    int y = bounds.y - scroll_y;
    if(align & R_BOTTOM)      y += bounds.h - block_h;
    else if(!(align & R_TOP))
    {
        // Centre the capitals rather than the ascender-to-descender block: tall ascenders would push the text visibly low.
        int cap = (__draw_face_cap_height(t->font->font) + 32) / 64;
        if(cap > 0) y += (bounds.h - (t->num_lines - 1) * line_height - cap) / 2 + cap - ascender;
        else        y += (bounds.h - block_h + 1) / 2;
    }

    typedef struct { uint32_t color; int dilate, blur, x, y; } pass;
    pass passes[3] = { { 0 }, { 0 }, { color } };
    if(fx && (fx->text_outline_color >> 24) && fx->text_outline > 0)
        passes[1] = (pass){ fx->text_outline_color, effect_px(fx->text_outline, s) };
    if(fx && (fx->text_shadow_color >> 24))
        passes[0] = (pass){ fx->text_shadow_color, passes[1].dilate, effect_px(fx->text_shadow_blur, s), (int)roundf(fx->text_shadow_x * s), (int)roundf(fx->text_shadow_y * s) };

    for(int p = 0; p < 3; p++)
    {
        if(!(passes[p].color >> 24)) continue;
        uint32_t tint = __draw_premult_color(passes[p].color), colored_tint = __draw_premult_color(passes[p].color | 0x00FFFFFF);
        for(int i = 0; i < t->num_glyphs; i++)
        {
            const laid_glyph* lg = &t->glyphs[i];
            int baseline = y + lg->line * line_height + ascender + passes[p].y;
            if(baseline - ascender > clip.y + clip.h || baseline + line_height < clip.y) continue;
            int line_w = (t->line_w[lg->line] + 32) >> 6, x = bounds.x - scroll_x + passes[p].x;
            if(align & R_RIGHT)         x += bounds.w - line_w;
            else if(align & R_H_CENTER) x += (bounds.w - line_w) / 2;
            x = ((x << 6) + lg->x + 32) >> 6;
            int gi = effect_glyph(c, lg->glyph, passes[p].dilate, passes[p].blur);
            if(gi < 0 || x > clip.x + clip.w || !__draw_glyph_pixels(c, gi)) continue;
            struct r_draw_glyph* g = &c->glyphs[gi];
            r_frect dst = { (float)(x + g->x_origin), (float)(baseline - ((lg->y + 32) >> 6) + g->y_origin), g->w, g->h };
            __draw_emit(c, g->handle, g->is_colored ? colored_tint : tint, (r_frect){ 0, 0, g->w, g->h }, dst, clip);
        }
    }
}

void __draw_render_text(r_draw* c, r_drawable* d, const r_draw_style_rule* rule, rect bounds, r_alignment align, rect clip, int wrap_width)
{
    r_draw_font* font = default_font(c, rule->label_font);
    if(!font || !d->text) return;
    wrap_width = (int)(wrap_width * c->scale);
    text_layout t = font->flags & R_FONT_SHAPED ? drawable_layout(c, d, font, wrap_width) : layout_text(c, font, d->text, (int)strlen(d->text), wrap_width);
    render_layout(c, &t, rule->label_color, rule, bounds, align, 0, 0, clip);
}

void r_draw_render_text_ex(r_draw* c, r_draw_font* font, const char* text, uint32_t color, rect bounds, r_alignment align, int scroll_x, int scroll_y, rect clip, int wrap_width)
{
    font = default_font(c, font);
    if(!font || !text) return;
    text_layout t = layout_text(c, font, text, (int)strlen(text), (int)(wrap_width * c->scale));
    render_layout(c, &t, color, NULL, bounds, align, scroll_x, scroll_y, clip);
}

void r_draw_render_text(r_draw* c, r_draw_font* font, const char* text, uint32_t color, rect bounds)
{
    r_draw_render_text_ex(c, font, text, color, bounds, R_TOP_LEFT, 0, 0, bounds, 0);
}

void r_draw_render_icon(r_draw* c, uint16_t icon, uint32_t color, rect bounds)
{
    rect px = __draw_to_pixels(bounds, c->scale);
    int gi = icon_glyph(c, icon, MIN(px.w, px.h));
    if(gi < 0 || !__draw_glyph_pixels(c, gi)) return;
    struct r_draw_glyph* g = &c->glyphs[gi];
    float x = floorf(px.x + (px.w - g->w) / 2), y = floorf(px.y + (px.h - g->h) / 2);
    uint32_t tint = __draw_premult_color(g->is_colored ? color | 0x00FFFFFF : color);
    __draw_emit(c, g->handle, tint, (r_frect){ 0, 0, g->w, g->h }, (r_frect){ x, y, g->w, g->h }, c->clip_px);
}
