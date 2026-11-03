#pragma once
#include "rcut_core.h"

// Text backend: RCUT_USE_HARFBUZZ, RCUT_USE_HB_MINI or RCUT_USE_STB_TRUETYPE, otherwise detected from previously included headers.
#if !defined(RCUT_USE_HARFBUZZ) && !defined(RCUT_USE_HB_MINI) && !defined(RCUT_USE_STB_TRUETYPE)
	#if defined(HB_MINI_H)
		#define RCUT_USE_HB_MINI
	#elif defined(HB_H)
		#define RCUT_USE_HARFBUZZ
	#else
		#define RCUT_USE_STB_TRUETYPE
	#endif
#endif

#define RCUT_DRAW_MAX_STYLEBOXES 512
#define RCUT_DRAW_USER_STYLEBOXES 256 // Stylebox slots from here on are for the application, themes only use the ones below.
#define RCUT_DRAW_MAX_GLYPHS 8192
#define RCUT_DRAW_GLYPH_LOAD_LIMIT (RCUT_DRAW_MAX_GLYPHS * 3 / 4) // The glyph tables count as full from here on, probing gets slow beyond it.

#define RCUT_DRAW_FONT_HASHMAP_SIZE 256

static inline uint32_t r_rgb_u8(uint8_t R, uint8_t G, uint8_t B){ return 0xFF000000u | ((uint32_t)(R) << 16) | ((uint32_t)(G) << 8) | (uint32_t)(B); }
static inline uint32_t r_rgba_u8(uint8_t R, uint8_t G, uint8_t B, uint8_t A){ return ((uint32_t)(A) << 24) | ((uint32_t)(R) << 16) | ((uint32_t)(G) << 8) | (uint32_t)(B); }
static inline uint32_t r_rgb_f(float R, float G, float B){ return r_rgb_u8((uint8_t)(R*255.0f), (uint8_t)(G*255.0f), (uint8_t)(B*255.0f)); }
static inline uint32_t r_rgba_f(float R, float G, float B, float A){ return r_rgba_u8((uint8_t)(R*255.0f), (uint8_t)(G*255.0f), (uint8_t)(B*255.0f), (uint8_t)(A*255.0f)); }
static inline uint32_t r_hex(uint32_t rgb){ return 0xFF000000u | rgb; }
static inline uint32_t r_hexa(uint32_t rgb, float alpha){ return ((uint32_t)(alpha * 255.0f + 0.5f) << 24) | (rgb & 0xFFFFFF); }

typedef struct r_frect { float x, y, w, h; } r_frect;

typedef struct r_draw_transform
{
	float m[2][3];
	rect  clip;
} r_draw_transform;

typedef struct r_draw_quad
{
	uint64_t handle;
	uint32_t color;
	r_frect  src;
	r_frect  dst;
	const r_draw_transform* transform;
} r_draw_quad;

typedef struct r_draw_backend
{
	void* userptr;
	uint64_t (*cache_argb)      (void* userptr, int w, int h, const uint32_t* pixels, int stride, bool permanent);
	uint64_t (*cache_monochrome)(void* userptr, int w, int h, const uint8_t* pixels, int stride, bool permanent);
	void     (*submit)          (void* userptr, const r_draw_quad* quads, int num_quads);
	bool     (*is_cached)       (void* userptr, uint64_t handle);
	void     (*invalidate_cache)(void* userptr, uint64_t handle);
} r_draw_backend;

typedef struct r_draw_atlas
{
	int      w, h;
	int      cur_y;
	uint16_t generation;
	bool     full;
	struct{ int remaining_w; int y; } rows[15];
} r_draw_atlas;

uint64_t r_draw_atlas_alloc   (r_draw_atlas* a, int w, int h);
bool     r_draw_atlas_is_valid(r_draw_atlas* a, uint64_t handle);
rect     r_draw_atlas_region  (uint64_t handle);
void     r_draw_atlas_reset   (r_draw_atlas* a);

typedef struct r_draw_software
{
	uint32_t* pixels;
	int       w, h, stride;
	int       x, y;
	uint32_t  clear_color;
	uint32_t* buffer;
	uint32_t* cache;
	uint64_t  cache_size;
	uint64_t  write_pos;
	uint64_t  frame_oldest;
	bool      full;
	int*      columns;
	int       num_columns;
	void**    permanent;
	int       num_permanent;
} r_draw_software;

void           r_draw_software_init   (r_draw_software* sw, int w, int h, int cache_size);
void           r_draw_software_free   (r_draw_software* sw);
r_draw_backend r_draw_software_backend(r_draw_software* sw);
void           r_draw_software_set_target(r_draw_software* sw, uint32_t* pixels, int w, int h, int stride, int x, int y);

typedef enum r_draw_font_flags{
	R_FONT_DEFAULT_WEIGHT = 0x00,
	R_FONT_LIGHTEST       = 0x01,
	R_FONT_LIGHTER        = 0x02,
	R_FONT_LIGHT          = 0x03,
	R_FONT_REGULAR        = 0x04,
	R_FONT_MEDIUM         = 0x05,
	R_FONT_SEMIBOLD       = 0x06,
	R_FONT_BOLD           = 0x07,
	R_FONT_BOLDER         = 0x08,
	R_FONT_BOLDEST        = 0x09,
	R_FONT_ULTRAHEAVY     = 0x0A,
	R_FONT_WEIGHT_MASK    = 0x0F,
	R_FONT_ITALIC         = 0x10,
	R_FONT_SHAPED         = 0x20, // Shaped with HarfBuzz: kerning, ligatures, combining marks and complex scripts. Without it codepoints map 1:1 to glyphs.
} r_draw_font_flags;

typedef struct r_draw_font
{
	int base_font_idx;
	int size;
	float scale;
	uint32_t flags;
	uint32_t resolved_flags;

	struct r_draw_face_inst* font;
} r_draw_font;

typedef struct r_draw_gradient
{
	uint32_t base_color;

	bool use_gradient:1;
	bool is_elliptical:1;
	unsigned int num_stops:3;

	struct{
		uint32_t color;
		float    position;
	} stops[8];

	float origin_x, origin_y;
	float end_x, end_y;
	// For elliptical gradients:
	float aspect_ratio;
	float focal_offset_x, focal_offset_y;
} r_draw_gradient;

typedef struct r_draw_stylebox_def
{
	float extra_padding[4];

	r_draw_gradient border_gradient[4];
	float border_thickness[4]; // Note: Should be handled inset by the rasterizer so it can be alpha blended with the background.
	float border_blur[4];      // Blurs the inner edge of the border (outer edge remains sharp)
	float corner_radius[4];

	struct r_draw_shadow {
		float offset_x, offset_y;
		float grow; // SDF offset.
		float blur; // SDF smooth threshold width.
		float hardness; // pow(value, hardness)
		uint32_t color;
		bool  is_inset:1; // Invert shadow SDF function and do alpha-over instead of alpha-under.
	} shadows[4];

	// If bg_preserve_aspect_ratio = true the stylebox should be (roughly) rectangular, meaning corner_radius <= border_thickness, or else you can get transparent pixels.
    // If bg_preserve_aspect_ratio = false the gradients should be (roughly) axis aligned or else you can get warped gradients.
    bool bg_preserve_aspect_ratio; 
    r_draw_gradient bg_gradients[4];
    int bg_raster_width;
    int bg_raster_height;
    float transparency;
} r_draw_stylebox_def;

void r_stylebox_set_outline_border(r_draw_stylebox_def* box, uint32_t color, float border_thickness, float border_radius);
void r_stylebox_set_shadow(r_draw_stylebox_def* box, int slot, float offset_x, float offset_y, float blur, float opacity, bool inset);
void r_stylebox_set_bg_color(r_draw_stylebox_def* box, uint32_t color);
void r_stylebox_set_bg_vertical_gradient(r_draw_stylebox_def* box, uint32_t* stops, int num_stops);
void r_stylebox_set_raised_borders_simple(r_draw_stylebox_def* box, float angle, uint32_t light_color, uint32_t ambient_color, float border_thickness, float border_radius, float blur);

typedef enum r_draw_icon_op
{
	R_ICON_OP_END = 0,
	R_ICON_OP_WIDTH,    // w              : Stroke width of the following shapes (default 2), 0 fills them instead
	R_ICON_OP_POLYLINE, // n x0 y0 ...    : Open polyline with n points, filled as a closed polygon
	R_ICON_OP_ARC,      // cx cy r a0 a1  : Arc from a0 to a1 (clockwise), filled as a full disc
	R_ICON_OP_RECT,     // x y w h r      : Rounded rectangle
	R_ICON_OP_COLOR,    // r g b a        : Color of the following shapes (0-1, default white). Icons with a color are not tinted.
} r_draw_icon_op;

typedef struct r_draw_icon_def
{
	const float* ops;
} r_draw_icon_def;

typedef struct r_draw_style_rule
{
	enum r_style style;

	int bg_stylebox;
	int overlay;
	r_draw_font* label_font;
	uint32_t    label_color;
	int icon_size;
	int size;
	int spacing;
	uint32_t    icon_color;
	r_alignment label_align;

	int focus_overlay;
	int hover_overlay;

	uint32_t text_outline_color;
	float    text_outline;
	uint32_t text_shadow_color;
	float    text_shadow_x, text_shadow_y, text_shadow_blur;
} r_draw_style_rule;

typedef struct r_draw_theme_options
{
	uint32_t background;
	uint32_t text;
	uint32_t primary;
	uint32_t checked;
	uint32_t highlight;
	uint32_t value;
	uint32_t selection;
	uint32_t focus;
	float    font_size;
	bool     scrollbar_buttons;
} r_draw_theme_options;

extern const r_draw_theme_options r_draw_default_theme_options;

typedef struct r_draw_palette
{
	bool     dark;
	float    light;
	uint32_t window, panel, view, card, input, menu, tooltip, tooltip_text;
	uint32_t control, control_hover, control_pressed, track, thumb;
	uint32_t border, border_strong, border_hover;
	uint32_t text, text_strong, text_dim;
	uint32_t primary, checked, highlight, value, selection, focus;
	uint32_t accent_text, selection_subtle;
	uint32_t hover_overlay;
} r_draw_palette;

typedef struct r_draw
{
	r_context*     context;
	r_draw_backend backend;
	float    scale;
	uint32_t clear_color;
	r_draw_palette palette;
	struct r_theme* theme;
	bool     cache_full;
	bool     glyphs_full;
	uint64_t white;
	void*    font_state; // Font backend scratch state.
	struct r_draw_text* text;

	r_draw_quad* quads;
	int num_quads, cap_quads;
	r_draw_transform transforms[RCUT_CONTEXT_MAX_LISTS + RCUT_CONTEXT_MAX_EPHEMERAL_LISTS];
	int num_transforms;

	rect area;
	bool drawn_caret;

	r_ui_list*  pass_list;
	r_drawable* pass_item;
	int  pass_ox, pass_oy;
	rect pass_clip;
	rect clip_px;
	const r_draw_transform* pass_transform;

	r_hashmap style_rules;

	struct r_draw_stylebox {
		r_draw_stylebox_def definition;
		bool      permanent, bg_permanent;
		uint64_t  handle, bg_handle;
		int   w, h, bg_w, bg_h;
		float source_scale;
		int   min_w, min_h;
		bool  has_separate_background; // Separate backgrounds are drawn scaled to rectangle size while preserving aspect ratio size. (Like "cover" in CSS.)
		int   side_width[4];  // 9 rect coordinates
		int   side_expand[4]; // How much to overdraw outside of the style rectangle.
		int   side_drawn[4];
		int   side_inset[4];  // How much content padding to add inside the style rectangle.
		int   side_bg_inset[4]; // How much separate backgrounds should be inset.
	} styleboxes[RCUT_DRAW_MAX_STYLEBOXES];

	r_draw_icon_def icons[R_ICON_USER+128];

	struct r_draw_sized_stylebox { int slot, w, h; uint64_t handle; } sized_styleboxes[64];
	int num_sized_styleboxes;

	// List: Base font
	struct r_draw_base_font{
		char*      path;
		struct r_draw_face* face;
		int8_t fallbacks[16];
		int8_t italic_redirect;
		int8_t bold_redirect;
		int16_t weight;
		bool lazy_load:1; // If true, font is loaded from path only when actually needed. This is primarily for fallback fonts.
		struct
		{
			uint32_t min;
			uint32_t max;
		} codepoint_ranges[8]; // If not 0, this fallback font is only used for codepoints in these ranges.
	} base_fonts[64]; // 0 is the basic font used by the default theme (Arial, Liberation Sans or Roboto, depending on which is installed on the OS), 32-64 are filled by the system font fallbacks.
	int8_t font_system_fallbacks[16];
	int    font_system_num_fallbacks;

	// Hashmap: Base font IDX + Font properties -> Font data
	r_draw_font fonts[RCUT_DRAW_FONT_HASHMAP_SIZE];

	// Hashmap: Font + Glyph ID -> Glyph data
	struct r_draw_glyph
	{
		int       font_id;
		uint16_t  glyph_id; // For icons: R_ICON_* constants
		int32_t   x_advance;
		bool      is_empty;
		int32_t   x_origin;
		int32_t   y_origin;
		int32_t   w, h;
		bool      is_colored;
		uint64_t  handle;
	} glyphs[RCUT_DRAW_MAX_GLYPHS];
	int num_glyphs;

	// Hashmap: Font + Codepoint -> Index into glyphs hashmap
	struct r_draw_glyph_mapping
	{
		int      font_id;
		uint32_t codepoint;
		int      glyph_idx;
	} glyph_mapping[RCUT_DRAW_MAX_GLYPHS];
	int num_glyph_mappings;
} r_draw;

void r_draw_init (r_draw* d, r_context* ctx, const r_draw_backend* backend, float scale);
void r_draw_free (r_draw* d);
void r_draw_render(r_draw* d, uint64_t target);
rect r_draw_take_damage(r_draw* d, uint64_t target);
void r_draw_render_area(r_draw* d, uint64_t target, rect area);
rect r_draw_target_bounds(r_draw* d, uint64_t target);
void r_theme_default(r_draw* c, const r_draw_theme_options* options);

//
//
//
enum r_theme_focus { R_FOCUS_NONE, R_FOCUS_RING };
void r_theme             (r_draw* c, int key);
void r_theme_tweak       (r_draw* c, int key);
void r_theme_make_box    (r_draw* c);
void r_theme_apply_states(r_draw* c, int states);
void r_theme_apply_joined(r_draw* c, int states);
void r_theme_fill        (r_draw* c, uint32_t top, uint32_t bottom);
void r_theme_surface     (r_draw* c, uint32_t color);
void r_theme_tint        (r_draw* c, uint32_t color);
void r_theme_border      (r_draw* c, float width, uint32_t color);
void r_theme_border_color(r_draw* c, uint32_t color);
void r_theme_underline   (r_draw* c, float width, uint32_t color);
void r_theme_bevel       (r_draw* c, uint32_t top, uint32_t bottom);
void r_theme_radius      (r_draw* c, float radius);
void r_theme_corners     (r_draw* c, float top_left, float top_right, float bottom_left, float bottom_right);
void r_theme_padding     (r_draw* c, float x, float y);
void r_theme_shadow      (r_draw* c, float y, float blur, float opacity);
void r_theme_shadow2     (r_draw* c, float y, float blur, float opacity);
void r_theme_no_shadow   (r_draw* c);
void r_theme_inset       (r_draw* c, float y, float blur, float opacity);
void r_theme_glow        (r_draw* c, uint32_t color, float width);
void r_theme_label       (r_draw* c, uint32_t color, r_alignment align);
void r_theme_label_color (r_draw* c, uint32_t color);
void r_theme_font        (r_draw* c, r_draw_font* font);
void r_theme_icon        (r_draw* c, uint32_t color, int size);
void r_theme_layout      (r_draw* c, int size, int spacing);
void r_theme_focus       (r_draw* c, enum r_theme_focus ring);
void r_theme_text_outline(r_draw* c, float width, uint32_t color);
void r_theme_text_shadow (r_draw* c, float x, float y, float blur, uint32_t color);
void r_draw_set_scale(r_draw* c, float scale);

void r_draw_clear_cache(r_draw* c);

r_draw_style_rule* r_draw_get_style_rule(r_draw* c, enum r_style key);
r_draw_style_rule* r_draw_find_style_rule(r_draw* c, int style);
void              r_draw_set_style_rule(r_draw* c, enum r_style key, r_draw_style_rule* rule);
void              r_draw_delete_style_rule(r_draw* c, enum r_style key);

// Styleboxes
bool r_draw_upload_stylebox     (r_draw* c, int slot, uint8_t* pixels, int w, int h, int pitch, int left, int right, int top, int bottom);
// Upload a aspect ratio aware fill to be used with the stylebox. It will be drawn under the rest of the stylebox. (The center of the stylebox must be transparent.)
bool r_draw_upload_stylebox_fill(r_draw* c, int slot, uint8_t* pixels, int w, int h, int pitch);
bool r_draw_generate_stylebox   (r_draw* c, int slot, r_draw_stylebox_def* definition);
void r_draw_free_stylebox       (r_draw* c, int slot);

// Given the outer rect, calculate the inner rect (e.g. where text should be positioned)
rect r_draw_stylebox_inner_rect(r_draw* c, int slot, rect outer);
// Given the inner rect, calculate the outer rect (e.g. how big the entire drawable needs to be to contain the contents of inner)
rect r_draw_stylebox_outer_rect(r_draw* c, int slot, rect* inner);

// Icons
bool r_draw_upload_icon  (r_draw* c, int slot, r_draw_icon_def* icon);
rect r_draw_icon_get_rect(r_draw* c, int slot, rect outer);
// Icons use the same underlying caching system as fonts, caching each "font size" of the icon as needed.

// Fonts
void r_draw_find_system_fonts(r_draw* c, bool include_base_fonts);
bool r_draw_font_register     (r_draw* c, int slot, const char* font_path, bool lazy);
void r_draw_font_set_fallbacks(r_draw* c, int base_font, int* fallbacks, int num_fallbacks, bool include_system_fallbacks);
r_draw_font* r_draw_get_font   (r_draw* c, int base_font, int size, uint32_t flags);
int  r_draw_font_line_height  (r_draw_font* font);
rect r_draw_measure_text      (r_draw* c, r_draw_font* font, const char* text, int len);
rect r_draw_measure_text_wrapped(r_draw* c, r_draw_font* font, const char* text, int len, int wrap_width);
int  r_draw_break_text        (r_draw* c, r_draw_font* font, const char* text, int len, int width, bool force, int* w);

// Text

void r_draw_render_pass_begin (r_draw* c, r_ui_list* target);
void r_draw_render_pass_end(r_draw* c);

void r_draw_render_drawable(r_draw* c, r_drawable* d);
void r_draw_render_icon    (r_draw* c, uint16_t icon, uint32_t color, rect bounds);
void r_draw_render_text    (r_draw* c, r_draw_font* font, const char* text, uint32_t color, rect bounds);
void r_draw_render_stylebox(r_draw* c, int stylebox, rect outer);
void r_draw_render_fill    (r_draw* c, uint32_t color, rect bounds);
void r_draw_render_text_ex (r_draw* c, r_draw_font* font, const char* text, uint32_t color, rect bounds, r_alignment align, int scroll_x, int scroll_y, rect clip, int wrap_width);