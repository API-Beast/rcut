#include "internal.h"

// Icons (24x24 grid, 2 unit strokes)

#define OP(...) (const float[]){ __VA_ARGS__, R_ICON_OP_END }
#define LINE(x0, y0, x1, y1)      R_ICON_OP_POLYLINE, 2, x0, y0, x1, y1
#define PLINE(n, ...)             R_ICON_OP_POLYLINE, n, __VA_ARGS__
#define ARC(cx, cy, r, a0, a1)    R_ICON_OP_ARC, cx, cy, r, a0, a1
#define CIRCLE(cx, cy, r)         ARC(cx, cy, r, 0, 360)
#define DISC(cx, cy, r)           WIDTH(0), CIRCLE(cx, cy, r) // Leaves the width at 0 (filled)
#define RECT(x, y, w, h, r)       R_ICON_OP_RECT, x, y, w, h, r
#define WIDTH(w)                  R_ICON_OP_WIDTH, w

#define CHEVRON_DOWN  PLINE(3, 6,9, 12,15, 18,9)
#define CHEVRON_UP    PLINE(3, 6,15, 12,9, 18,15)
#define CHEVRON_LEFT  PLINE(3, 15,18, 9,12, 15,6)
#define CHEVRON_RIGHT PLINE(3, 9,18, 15,12, 9,6)
#define MAGNIFIER     CIRCLE(11,11,7), LINE(21,21, 16,16)

static const float* default_icons[R_ICON_USER] =
{
	[R_ICON_DROPDOWN_INDICATOR] = OP(CHEVRON_DOWN),
	[R_ICON_UP]           = OP(CHEVRON_UP),
	[R_ICON_DOWN]         = OP(CHEVRON_DOWN),
	[R_ICON_LEFT]         = OP(CHEVRON_LEFT),
	[R_ICON_RIGHT]        = OP(CHEVRON_RIGHT),
	[R_ICON_BACK]         = OP(LINE(19,12, 5,12), PLINE(3, 12,19, 5,12, 12,5)),
	[R_ICON_MORE]         = OP(DISC(5,12,1.6f), DISC(12,12,1.6f), DISC(19,12,1.6f)),
	[R_ICON_EXPANDED]     = OP(RECT(3,3,18,18,3), LINE(8,12, 16,12)),
	[R_ICON_COLLAPSED]    = OP(RECT(3,3,18,18,3), LINE(8,12, 16,12), LINE(12,8, 12,16)),
	[R_ICON_SHOW_DETAILS] = OP(CHEVRON_RIGHT),
	[R_ICON_HIDE_DETAILS] = OP(CHEVRON_DOWN),
	[R_ICON_ADD]          = OP(LINE(12,5, 12,19), LINE(5,12, 19,12)),
	[R_ICON_SUBTRACT]     = OP(LINE(5,12, 19,12)),
	[R_ICON_EDIT]         = OP(PLINE(6, 17,3, 21,7, 7.5f,20.5f, 2.5f,21.5f, 3.5f,16.5f, 17,3), LINE(15,5, 19,9)),
	[R_ICON_SEARCH]       = OP(MAGNIFIER),
	[R_ICON_RELOAD]       = OP(ARC(12,12,9, 0,270), PLINE(3, 12,3, 18.4f,5.6f, 21,8), PLINE(3, 21,3, 21,8, 16,8)),
	[R_ICON_ZOOM_IN]      = OP(MAGNIFIER, LINE(11,8, 11,14), LINE(8,11, 14,11)),
	[R_ICON_ZOOM_OUT]     = OP(MAGNIFIER, LINE(8,11, 14,11)),
	[R_ICON_ROTATE_LEFT]  = OP(ARC(12,12,9, 270,180), PLINE(3, 12,3, 5.6f,5.6f, 3,8), PLINE(3, 3,3, 3,8, 8,8)),
	[R_ICON_ROTATE_RIGHT] = OP(ARC(12,12,9, 0,270), PLINE(3, 12,3, 18.4f,5.6f, 21,8), PLINE(3, 21,3, 21,8, 16,8)),
	[R_ICON_LOCK]         = OP(RECT(4,11,16,10,2), LINE(8,11, 8,7), LINE(16,7, 16,11), ARC(12,7,4, 180,360)),
	[R_ICON_UNLOCK]       = OP(RECT(4,11,16,10,2), LINE(8,11, 8,7), ARC(12,7,4, 180,345)),
	[R_ICON_CHECKED]      = OP(WIDTH(3), PLINE(3, 20,6, 9.5f,16.5f, 4,11)),
	[R_ICON_UNCHECKED]    = OP(RECT(3,3,18,18,3)),
	[R_ICON_MARKED]       = OP(DISC(12,12,12)),
	[R_ICON_INFO]         = OP(CIRCLE(12,12,10), LINE(12,16, 12,12), DISC(12,8,1.25f)),
	[R_ICON_WARNING]      = OP(PLINE(4, 12,3.5f, 21.5f,20, 2.5f,20, 12,3.5f), LINE(12,9.5f, 12,13.5f), DISC(12,16.8f,1.25f)),
	[R_ICON_ERROR]        = OP(CIRCLE(12,12,10), LINE(15,9, 9,15), LINE(9,9, 15,15)),
	[R_ICON_MINIMIZE]     = OP(WIDTH(1.5f), LINE(6,12, 18,12)),
	[R_ICON_MAXIMIZE]     = OP(WIDTH(1.5f), RECT(6,6,12,12,1)),
	[R_ICON_UNMAXIMIZE]   = OP(WIDTH(1.5f), RECT(5,8,11,11,1), PLINE(5, 8,8, 8,5, 19,5, 19,16, 16,16)),
	[R_ICON_CLOSE]        = OP(LINE(18,6, 6,18), LINE(6,6, 18,18)),
	[R_ICON_MENU]         = OP(LINE(4,6, 20,6), LINE(4,12, 20,12), LINE(4,18, 20,18)),
	[R_ICON_SETTINGS]     = OP(PLINE(33, 18.81f,9.66f, 21.85f,10.26f, 21.85f,13.74f, 18.81f,14.34f, 18.47f,15.16f, 20.19f,17.74f, 17.74f,20.19f, 15.16f,18.47f, 14.34f,18.81f, 13.74f,21.85f, 10.26f,21.85f, 9.66f,18.81f, 8.84f,18.47f, 6.26f,20.19f, 3.81f,17.74f, 5.53f,15.16f, 5.19f,14.34f, 2.15f,13.74f, 2.15f,10.26f, 5.19f,9.66f, 5.53f,8.84f, 3.81f,6.26f, 6.26f,3.81f, 8.84f,5.53f, 9.66f,5.19f, 10.26f,2.15f, 13.74f,2.15f, 14.34f,5.19f, 15.16f,5.53f, 17.74f,3.81f, 20.19f,6.26f, 18.47f,8.84f, 18.81f,9.66f), CIRCLE(12,12,3)),
	[R_ICON_HOME]         = OP(PLINE(3, 3,10, 12,3, 21,10), PLINE(4, 5,9, 5,21, 19,21, 19,9), PLINE(4, 9.5f,21, 9.5f,14, 14.5f,14, 14.5f,21)),
	[R_ICON_FOLDER]       = OP(PLINE(7, 2,4, 9,4, 11,7, 22,7, 22,20, 2,20, 2,4)),
	[R_ICON_FILE]         = OP(PLINE(6, 15,2, 4,2, 4,22, 20,22, 20,7, 15,2), PLINE(3, 14,2, 14,8, 20,8)),
	[R_ICON_SAVE]         = OP(PLINE(6, 3,3, 16,3, 21,8, 21,21, 3,21, 3,3), PLINE(4, 17,21, 17,13, 7,13, 7,21), PLINE(3, 7,3, 7,8, 15,8)),
	[R_ICON_TRASH]        = OP(LINE(3,6, 21,6), PLINE(4, 19,6, 19,21, 5,21, 5,6), PLINE(4, 8,6, 8,3, 16,3, 16,6), LINE(10,11, 10,16), LINE(14,11, 14,16)),
	[R_ICON_COPY]         = OP(RECT(8,8,14,14,2), PLINE(5, 4,16, 2,16, 2,2, 16,2, 16,4)),
	[R_ICON_CUT]          = OP(CIRCLE(6,6,3), CIRCLE(6,18,3), LINE(20,4, 8.12f,15.88f), LINE(14.47f,14.48f, 20,20), LINE(8.12f,8.12f, 12,12)),
	[R_ICON_PASTE]        = OP(RECT(8,2,8,4,1), PLINE(6, 16,4, 20,4, 20,22, 4,22, 4,4, 8,4)),
	[R_ICON_UNDO]         = OP(PLINE(3, 9,14, 4,9, 9,4), LINE(4,9, 14.5f,9), ARC(14.5f,14.5f,5.5f, 270,90), LINE(14.5f,20, 11,20)),
	[R_ICON_REDO]         = OP(PLINE(3, 15,14, 20,9, 15,4), LINE(20,9, 9.5f,9), ARC(9.5f,14.5f,5.5f, 90,270), LINE(9.5f,20, 13,20)),
	[R_ICON_STAR]         = OP(PLINE(11, 12.00f,2.80f, 14.53f,9.32f, 21.51f,9.71f, 16.09f,14.13f, 17.88f,20.89f, 12.00f,17.10f, 6.12f,20.89f, 7.91f,14.13f, 2.49f,9.71f, 9.47f,9.32f, 12.00f,2.80f)),
	[R_ICON_HEART]        = OP(ARC(8,9,4.5f, 135,333), ARC(16,9,4.5f, 207,45), PLINE(3, 4.8f,12.2f, 12,20, 19.2f,12.2f)),
	[R_ICON_BELL]         = OP(ARC(12,8,6, 180,360), PLINE(6, 6,8, 6,12, 3.5f,16.5f, 20.5f,16.5f, 18,12, 18,8), ARC(12,19,2.2f, 0,180)),
	[R_ICON_PLAY]         = OP(PLINE(4, 6,3, 20,12, 6,21, 6,3)),
	[R_ICON_PAUSE]        = OP(RECT(6,4,4,16,1), RECT(14,4,4,16,1)),
	[R_ICON_EYE]          = OP(ARC(12,15.64f,10.64f, 200,340), ARC(12,8.36f,10.64f, 20,160), CIRCLE(12,12,3)),
	[R_ICON_PERSON]       = OP(CIRCLE(12,7,4), ARC(9,19,4, 180,270), ARC(15,19,4, 270,360), LINE(5,19, 5,21), LINE(19,19, 19,21), LINE(9,15, 15,15)),
};

//

const r_draw_theme_options r_draw_default_theme_options =
{
	.background = 0xFF222226, .primary = 0xFFE8305E,
	.font_size = 14,
};

#define WHITE 0xFFFFFFFF
#define BLACK 0xFF000000

static inline uint32_t alpha(uint32_t c, float a){ return (c & 0xFFFFFF) | ((uint32_t)(a * 255.0f + 0.5f) << 24); }

static float to_linear(uint32_t c, int shift){ float v = ((c >> shift) & 0xFF) / 255.0f; return v <= 0.04045f ? v / 12.92f : powf((v + 0.055f) / 1.055f, 2.4f); }
static uint32_t from_linear(float v, int shift){ v = v <= 0.0031308f ? v * 12.92f : 1.055f * powf(v, 1 / 2.4f) - 0.055f; return (uint32_t)(fminf(fmaxf(v, 0), 1) * 255.0f + 0.5f) << shift; }
static float luminance(uint32_t c){ return 0.2126f * to_linear(c, 16) + 0.7152f * to_linear(c, 8) + 0.0722f * to_linear(c, 0); }
static float lightness(float y){ return y > 0.008856f ? 116 * cbrtf(y) - 16 : 903.3f * y; }

static uint32_t mix(uint32_t a, uint32_t b, float t)
{
	uint32_t r = (uint32_t)((a >> 24) + ((float)(b >> 24) - (float)(a >> 24)) * t + 0.5f) << 24;
	for(int i = 0; i < 24; i += 8) r |= from_linear(to_linear(a, i) + (to_linear(b, i) - to_linear(a, i)) * t, i);
	return r;
}

static uint32_t text_on(uint32_t s, int strength)
{
	static const float amount[2][3] = { { 0.27f, 0.66f, 0.83f }, { 0.89f, 0.97f, 0.985f } };
	bool light = lightness(luminance(s)) >= 50;
	return mix(s | 0xFF000000, light ? BLACK : WHITE, amount[light][strength]);
}

static uint32_t text_on_accent(uint32_t s){ return lightness(luminance(s)) >= 65 ? text_on(s, 2) : WHITE; }

static void grays(r_draw_palette* p, uint32_t bg, bool light)
{
	#define LIGHTER(a) mix(bg, WHITE, a)
	#define DARKER(a)  mix(bg, BLACK, a)
	if(!light)
	{
		p->window = DARKER(0.5f); p->panel = p->view = bg; p->card = LIGHTER(0.0017f);
		p->input = DARKER(0.54f); p->menu = LIGHTER(0.005f); p->tooltip = LIGHTER(0.0105f);
		p->control = LIGHTER(0.0163f); p->control_hover = LIGHTER(0.024f); p->control_pressed = LIGHTER(0.005f); p->track = LIGHTER(0.03f); // Lighter: a dark track vanishes on the window.
		p->border = LIGHTER(0.024f); p->border_strong = LIGHTER(0.039f); p->border_hover = LIGHTER(0.075f);
		p->thumb = mix(bg, WHITE, 0.9f);
	}
	else
	{
		p->window = DARKER(0.18f); p->panel = bg; p->view = WHITE; p->card = DARKER(0.037f);
		p->input = WHITE; p->menu = bg; p->tooltip = p->text_strong;
		p->control = DARKER(0.037f); p->control_hover = DARKER(0.1f); p->control_pressed = DARKER(0.2f); p->track = DARKER(0.37f);
		p->border = DARKER(0.37f); p->border_strong = DARKER(0.45f); p->border_hover = DARKER(0.6f);
		p->thumb = WHITE;
	}
	#undef LIGHTER
	#undef DARKER
}

static r_draw_palette make_palette(const r_draw_theme_options* o)
{
	uint32_t bg = o->background | 0xFF000000;
	float l = lightness(luminance(bg));
	float t = fminf(fmaxf((l - 30) / 40, 0), 1);
	r_draw_palette p = { .dark = l < 50, .light = t * t * (3 - 2 * t) };
	p.text = o->text ? o->text | 0xFF000000 : text_on(bg, 1);
	p.text_strong = o->text ? mix(p.text, p.dark ? WHITE : BLACK, 0.5f) : text_on(bg, 2);
	p.text_dim = text_on(bg, 0);
	p.hover_overlay = alpha(p.text_strong, 0.05f);

	r_draw_palette light = p;
	grays(&p, bg, false);
	grays(&light, bg, true);
	#define BLEND(f) p.f = mix(p.f, light.f, p.light);
	BLEND(window) BLEND(panel) BLEND(view) BLEND(card) BLEND(input) BLEND(menu) BLEND(tooltip)
	BLEND(control) BLEND(control_hover) BLEND(control_pressed) BLEND(track) BLEND(thumb)
	BLEND(border) BLEND(border_strong) BLEND(border_hover)
	#undef BLEND
	p.tooltip_text = text_on(p.tooltip, 2);

	p.primary = o->primary | 0xFF000000; p.checked = o->checked | 0xFF000000; p.highlight = o->highlight | 0xFF000000;
	p.value = o->value | 0xFF000000; p.selection = o->selection | 0xFF000000; p.focus = o->focus | 0xFF000000;
	p.accent_text = text_on_accent(p.primary);
	p.selection_subtle = alpha(p.selection, 0.22f - 0.08f * p.light);
	return p;
}

static inline float dl(const r_draw_palette* p, float dark, float light){ return dark + (light - dark) * p->light; }
static inline uint32_t dlc(const r_draw_palette* p, uint32_t dark, uint32_t light){ return mix(dark, light, p->light); }

typedef struct shadow_desc { float x, y, blur, grow; uint32_t color; } shadow_desc;

typedef struct box
{
	uint32_t bg, bg2;
	uint32_t surface;
	uint32_t border;
	uint32_t border_top, border_bottom;
	float    border_w;
	float    border_top_w, border_bottom_w;
	float    radius;
	float    corners[4];
	float    pad_x, pad_y;
	uint32_t hover, focus;
	shadow_desc shadow, shadow2, inset;
} box;

typedef struct theme_op { int type; float f[4]; uint32_t u[2]; r_draw_font* font; bool fixed; } theme_op; // fixed: color is used as given, not adapted to the background.
typedef struct theme_decl { int key, num_ops; theme_op ops[12]; } theme_decl;

typedef struct r_theme
{
	r_draw* c;
	int next_slot;
	r_draw_font *font, *bold, *small, *header;
	uint32_t focus_ring;
	int control_h;
	bool custom_text;
	bool dirty;
	theme_decl* cur;
	theme_decl decls[128];
	int num_decls;
} theme_state;

static inline float side_width(float w, float base){ return w < 0 ? 0 : w > 0 ? w : base; }

static int add_box(theme_state* t, const r_draw_stylebox_def* d)
{
	static const r_draw_stylebox_def none;
	if(!memcmp(d, &none, sizeof(*d))) return 0;

	for(int i = 1; i < t->next_slot; i++)
	if(!memcmp(&t->c->styleboxes[i].definition, d, sizeof(*d)))
		return i;

	if(t->next_slot >= RCUT_DRAW_USER_STYLEBOXES) { assert(false && "theme stylebox slots exhausted"); return 0; }
	int slot = t->next_slot++;
	r_draw_generate_stylebox(t->c, slot, (r_draw_stylebox_def*)d);
	return slot;
}

static int box_slot(theme_state* t, box b)
{
	if(!b.bg && !b.bg2 && b.border_w <= 0 && b.border_top_w <= 0 && b.border_bottom_w <= 0 && !b.shadow.color && !b.shadow2.color && !b.inset.color) return 0;
	r_draw_stylebox_def d;
	memset(&d, 0, sizeof(d));
	float bw = b.border_w > 0 ? b.border_w : 0;
	bool any_corner = b.corners[0] || b.corners[1] || b.corners[2] || b.corners[3];
	for(int i = 0; i < 4; i++)
	{
		d.corner_radius[i] = any_corner ? b.corners[i] : b.radius;
		d.border_thickness[i] = bw;
		d.border_gradient[i].base_color = b.border;
	}
	if(b.border_top)    d.border_gradient[1].base_color = b.border_top;
	if(b.border_bottom) d.border_gradient[3].base_color = b.border_bottom;
	d.border_thickness[1] = side_width(b.border_top_w, bw);
	d.border_thickness[3] = side_width(b.border_bottom_w, bw);
	if(b.border_top && b.border_top == b.bg)       d.border_thickness[1] = 0;
	if(b.border_bottom && b.border_bottom == (b.bg2 ? b.bg2 : b.bg)) d.border_thickness[3] = 0;

	r_stylebox_set_bg_color(&d, b.bg);
	if(b.bg2 && b.bg2 != b.bg)
	{
		r_stylebox_set_bg_vertical_gradient(&d, (uint32_t[]){b.bg, b.bg2}, 2);
		float edge = fmaxf(fmaxf(d.corner_radius[0], d.corner_radius[2]), bw) + fmaxf(fmaxf(d.corner_radius[1], d.corner_radius[3]), bw);
		d.bg_raster_height = (int)fmaxf(t->control_h - edge, 4);
	}

	shadow_desc* s[3] = { &b.shadow, &b.shadow2, &b.inset };
	for(int i = 0; i < 3; i++)
	{
		if(!s[i]->color) continue;
		d.shadows[i].offset_x = s[i]->x;
		d.shadows[i].offset_y = s[i]->y;
		d.shadows[i].blur     = s[i]->blur;
		d.shadows[i].grow     = s[i]->grow;
		d.shadows[i].color    = s[i]->color;
		d.shadows[i].is_inset = i == 2;
	}

	return add_box(t, &d);
}

static inline shadow_desc drop(float y, float blur, float opacity){ return (shadow_desc){ 0, y, blur, 0, r_rgba_f(0, 0, 0, opacity) }; }
static inline shadow_desc ring(uint32_t color, float width){ return (shadow_desc){ 0, 0, 1, width, color }; }

static uint32_t over_straight(uint32_t dst, uint32_t src)
{
	float sa = (src >> 24) / 255.0f, da = (dst >> 24) / 255.0f * (1 - sa), a = sa + da;
	if(a <= 0) return 0;
	uint32_t r = (uint32_t)(a * 255.0f + 0.5f) << 24;
	for(int i = 0; i < 24; i += 8) r |= (uint32_t)((((src >> i) & 0xFF) * sa + ((dst >> i) & 0xFF) * da) / a + 0.5f) << i;
	return r;
}

static uint32_t deepen(uint32_t c, float a)
{
	float m = 0;
	for(int i = 0; i < 24; i += 8) m = fmaxf(m, (float)((c >> i) & 0xFF));
	if(m <= 0) return c;
	uint32_t r = c & 0xFF000000;
	for(int i = 0; i < 24; i += 8) r |= (uint32_t)(m * (1 - a) * powf(((c >> i) & 0xFF) / m, 1 + a * 2) + 0.5f) << i;
	return r;
}

enum { OP_FILL, OP_SURFACE, OP_BORDER, OP_BORDER_COLOR, OP_UNDERLINE, OP_BEVEL, OP_RADIUS, OP_CORNERS, OP_PADDING, OP_SHADOW, OP_SHADOW2,
	OP_NO_SHADOW, OP_INSET, OP_GLOW, OP_TINT, OP_HOVER, OP_LABEL, OP_LABEL_COLOR, OP_FONT, OP_ICON, OP_LAYOUT, OP_FOCUS, OP_TEXT_OUTLINE, OP_TEXT_SHADOW };

static theme_state* theme(r_draw* c){ assert(c->theme && "load a theme first"); return c->theme; }

static void push(r_draw* c, theme_op op)
{
	theme_decl* d = theme(c)->cur;
	assert(d && "start a declaration with r_theme()");
	if(!d) return;
	int n = 0;
	for(int i = 0; i < d->num_ops; i++) if(d->ops[i].type != op.type) d->ops[n++] = d->ops[i];
	if(n >= COUNTOF(d->ops)) { assert(false && "too many properties in one declaration"); return; }
	d->ops[n++] = op;
	d->num_ops = n;
}

static void open_decl(r_draw* c, int key, bool clear)
{
	theme_state* t = theme(c);
	theme_decl* d = NULL;
	for(int i = 0; i < t->num_decls && !d; i++) if(t->decls[i].key == key) d = &t->decls[i];
	if(!d && t->num_decls < COUNTOF(t->decls)) { d = &t->decls[t->num_decls++]; clear = true; }
	assert(d && "theme declarations exhausted");
	if(d && clear) *d = (theme_decl){ .key = key };
	t->cur = d;
}

void r_theme(r_draw* c, int key){ open_decl(c, key, true); }
void r_theme_tweak(r_draw* c, int key){ open_decl(c, key, false); }

void r_theme_surface(r_draw* c, uint32_t color){ push(c, (theme_op){ OP_SURFACE, .u = {color} }); }
void r_theme_fill(r_draw* c, uint32_t top, uint32_t bottom){ push(c, (theme_op){ OP_FILL, .u = {top, bottom} }); }
void r_theme_border(r_draw* c, float width, uint32_t color){ push(c, (theme_op){ OP_BORDER, .f = {width}, .u = {color} }); }
void r_theme_border_color(r_draw* c, uint32_t color){ push(c, (theme_op){ OP_BORDER_COLOR, .u = {color} }); }
void r_theme_underline(r_draw* c, float width, uint32_t color){ push(c, (theme_op){ OP_UNDERLINE, .f = {width}, .u = {color} }); }
void r_theme_bevel(r_draw* c, uint32_t top, uint32_t bottom){ push(c, (theme_op){ OP_BEVEL, .u = {top, bottom} }); }
void r_theme_radius(r_draw* c, float radius){ push(c, (theme_op){ OP_RADIUS, .f = {radius} }); }
void r_theme_corners(r_draw* c, float top_left, float top_right, float bottom_left, float bottom_right){ push(c, (theme_op){ OP_CORNERS, .f = {top_left, top_right, bottom_left, bottom_right} }); }
void r_theme_padding(r_draw* c, float x, float y){ push(c, (theme_op){ OP_PADDING, .f = {x, y} }); }
void r_theme_shadow(r_draw* c, float y, float blur, float opacity){ push(c, (theme_op){ OP_SHADOW, .f = {y, blur, opacity} }); }
void r_theme_shadow2(r_draw* c, float y, float blur, float opacity){ push(c, (theme_op){ OP_SHADOW2, .f = {y, blur, opacity} }); }
void r_theme_no_shadow(r_draw* c){ push(c, (theme_op){ OP_NO_SHADOW }); }
void r_theme_inset(r_draw* c, float y, float blur, float opacity){ push(c, (theme_op){ OP_INSET, .f = {y, blur, opacity} }); }
void r_theme_glow(r_draw* c, uint32_t color, float width){ push(c, (theme_op){ OP_GLOW, .f = {width}, .u = {color} }); }
void r_theme_tint(r_draw* c, uint32_t color){ push(c, (theme_op){ OP_TINT, .u = {color} }); }
void r_theme_hover(r_draw* c, uint32_t color){ push(c, (theme_op){ OP_HOVER, .u = {color} }); }
void r_theme_label(r_draw* c, uint32_t color, r_alignment align){ push(c, (theme_op){ OP_LABEL, .u = {color, align} }); }
void r_theme_label_color(r_draw* c, uint32_t color){ push(c, (theme_op){ OP_LABEL_COLOR, .u = {color}, .fixed = true }); }
void r_theme_label_color_auto(r_draw* c, uint32_t color){ push(c, (theme_op){ OP_LABEL_COLOR, .u = {color} }); }
void r_theme_font(r_draw* c, r_draw_font* font){ push(c, (theme_op){ OP_FONT, .font = font }); }
void r_theme_icon(r_draw* c, uint32_t color, int size){ push(c, (theme_op){ OP_ICON, .u = {color, size} }); }
void r_theme_layout(r_draw* c, int size, int spacing){ push(c, (theme_op){ OP_LAYOUT, .u = {size, spacing} }); }
void r_theme_focus(r_draw* c, enum r_theme_focus ring){ push(c, (theme_op){ OP_FOCUS, .u = {ring} }); }
void r_theme_focus_border(r_draw* c, uint32_t color){ push(c, (theme_op){ OP_FOCUS, .u = {R_FOCUS_BORDER, color} }); }
void r_theme_text_outline(r_draw* c, float width, uint32_t color){ push(c, (theme_op){ OP_TEXT_OUTLINE, .f = {width}, .u = {color} }); }
void r_theme_text_shadow(r_draw* c, float x, float y, float blur, uint32_t color){ push(c, (theme_op){ OP_TEXT_SHADOW, .f = {x, y, blur}, .u = {color} }); }

static void run(theme_state* t, box* b, r_draw_style_rule* r, const theme_op* o)
{
	const float* f = o->f;
	const uint32_t* u = o->u;
	switch(o->type)
	{
	case OP_FILL:         b->bg = u[0]; b->bg2 = u[1]; break;
	case OP_SURFACE:      b->surface = u[0]; break;
	case OP_BORDER:       b->border_w = f[0]; b->border = u[0]; break;
	case OP_BORDER_COLOR: b->border = u[0]; break;
	case OP_UNDERLINE:    b->border_w = -1; b->border_bottom_w = f[0]; b->border = u[0]; break;
	case OP_BEVEL:        b->border_top = u[0]; b->border_bottom = u[1]; break;
	case OP_RADIUS:       b->radius = f[0]; break;
	case OP_CORNERS:
		for(int i = 0; i < 4; i++) b->corners[i] = f[i] ? f[i] : 0.001f;
		break;
	case OP_PADDING:      b->pad_x = f[0]; b->pad_y = f[1]; break;
	case OP_SHADOW:       b->shadow = drop(f[0], f[1], f[2]); break;
	case OP_SHADOW2:      b->shadow2 = drop(f[0], f[1], f[2]); break;
	case OP_NO_SHADOW:    b->shadow = b->shadow2 = (shadow_desc){0}; break;
	case OP_INSET:        b->inset = drop(f[0], f[1], f[2]); break;
	case OP_GLOW:         *(b->shadow.color ? &b->shadow2 : &b->shadow) = ring(u[0], f[0]); break;
	case OP_TINT:
		b->bg = over_straight(b->bg, u[0]);
		if(b->bg2) b->bg2 = over_straight(b->bg2, u[0]);
		if(b->border) b->border = over_straight(b->border, u[0]);
		if(b->border_top) b->border_top = over_straight(b->border_top, u[0]);
		if(b->border_bottom) b->border_bottom = over_straight(b->border_bottom, u[0]);
		break;
	case OP_HOVER:        b->hover = u[0]; break;
	case OP_LABEL:        r->label_color = u[0]; r->label_align = (r_alignment)u[1]; break;
	case OP_LABEL_COLOR:  r->label_color = u[0]; break;
	case OP_FONT:         r->label_font = o->font; break;
	case OP_ICON:         r->icon_color = u[0]; r->icon_size = (int)u[1]; break;
	case OP_LAYOUT:       r->size = (int)u[0]; r->spacing = (int)u[1]; break;
	case OP_FOCUS:        r->focus_overlay = u[0]; b->focus = u[1]; break;
	case OP_TEXT_OUTLINE: r->text_outline = f[0]; r->text_outline_color = u[0]; break;
	case OP_TEXT_SHADOW:  r->text_shadow_x = f[0]; r->text_shadow_y = f[1]; r->text_shadow_blur = f[2]; r->text_shadow_color = u[0]; break;
	}
}

static bool applies(int decl_key, int key)
{
	static const int fields[] = { R_STYLE_BASE_MASK, R_VARIANT_MASK, R_COMPONENT_MASK };

	for(int i = 0; i < COUNTOF(fields); i++)
	if((decl_key & fields[i]) && (decl_key & fields[i]) != (key & fields[i]))
		return false;

	return !(decl_key & (R_STATE_MASK | R_FLAG_MASK) & ~key);
}

static int order(int key)
{
	static const int states[] = { R_STATE_TOGGLED, R_STATE_SELECTED, R_STATE_FOCUS, R_STATE_HOVER, R_STATE_PRESSED, R_STATE_DISABLED };
	int o = 0;
	for(int i = 0; i < COUNTOF(states); i++) if(key & states[i]) o |= 1 << i;
	return o * 16 + !!(key & R_STYLE_BASE_MASK) + 2 * !!(key & R_COMPONENT_MASK) + 4 * !!(key & R_VARIANT_MASK) + 8 * !!(key & R_FLAG_MASK);
}

static int collect(theme_state* t, int key, theme_decl** list)
{
	int n = 0;
	for(int i = 0; i < t->num_decls; i++)
	{
		theme_decl* d = &t->decls[i];
		if(!applies(d->key, key)) continue;
		int j = n++;
		for(; j > 0 && order(list[j - 1]->key) > order(d->key); j--) list[j] = list[j - 1];
		list[j] = d;
	}
	return n;
}

static bool hover_only(const theme_decl* d)
{
	for(int i = 0; i < d->num_ops; i++) if(d->ops[i].type != OP_HOVER) return false;
	return true;
}

static box shape(const box* b)
{
	box s = { .radius = b->radius };
	memcpy(s.corners, b->corners, sizeof(s.corners));
	return s;
}

static void make(theme_state* t, int key, bool hover)
{
	theme_decl* list[COUNTOF(t->decls)];
	int n = collect(t, key | (hover ? R_STATE_HOVER : 0), list);
	box b = {0};
	r_draw_style_rule r = {0};
	bool fixed_label = false;

	uint32_t hover_color = 0;
	for(int i = 0; i < n; i++)
	for(int k = 0; k < list[i]->num_ops; k++)
	{
		const theme_op* o = &list[i]->ops[k];
		if(o->type == OP_HOVER) { hover_color = o->u[0]; continue; }
		if(!applies(list[i]->key, key)) continue;
		run(t, &b, &r, o);
		if(o->type == OP_LABEL || o->type == OP_LABEL_COLOR) fixed_label = o->fixed;
	}

	int comp = key & R_COMPONENT_MASK;
	if(comp == R_COMPONENT_ENDCAP_START || comp == R_COMPONENT_JOINED || comp == R_COMPONENT_ENDCAP_END)
	{
		if(!(b.corners[0] || b.corners[1] || b.corners[2] || b.corners[3]))
		for(int i = 0; i < 4; i++)
			b.corners[i] = b.radius;

		if(comp != R_COMPONENT_ENDCAP_START) b.corners[0] = b.corners[2] = 0.001f;
		if(comp != R_COMPONENT_ENDCAP_END)   b.corners[1] = b.corners[3] = 0.001f;
	}

	const r_draw_palette* p = &t->c->palette;
	uint32_t surface = (b.bg >> 24) >= 0x80 ? b.bg : b.surface ? b.surface : p->panel;
	uint32_t* colors[] = { &r.label_color, &r.icon_color };
	bool fixed[] = { fixed_label, false };
	for(int i = 0; i < 2 && !t->custom_text; i++)
	if(!fixed[i])
	{
		uint32_t roles[] = { p->text_dim, p->text, p->text_strong };
		for(int k = 0; k < 3; k++) if(*colors[i] == roles[k]) { *colors[i] = text_on(surface, k); break; }
	}

	for(int i = 0; i < 2; i++)
	if(!fixed[i] && *colors[i] && (*colors[i] & 0xFFFFFF) == (p->accent_text & 0xFFFFFF))
		*colors[i] = (text_on_accent(surface) & 0xFFFFFF) | (*colors[i] & 0xFF000000);

	if(r.focus_overlay)
	{
		box f = shape(&b);
		if(r.focus_overlay == R_FOCUS_RING) f.shadow = ring(t->focus_ring, 2);
		else { f.border_w = b.border_w > 0 ? b.border_w : 1; f.border = b.focus; }
		r.focus_overlay = box_slot(t, f);
	}
	if(hover && hover_color)
	{
		box h = shape(&b);
		h.bg = hover_color;
		r.hover_overlay = box_slot(t, h);
	}
	r.padding_x = (int)roundf(b.pad_x);
	r.padding_y = (int)roundf(b.pad_y);

	r.style = key;
	r.bg_stylebox = box_slot(t, b);
	r_draw_set_style_rule(t->c, key, &r);
	t->dirty = true;
}

static void make_base(theme_state* t, int key)
{
	r_hashmap* map = &t->c->style_rules;
	for(int i = 0; i < map->num_slots; i++)
	{
		r_draw_style_rule* r = hash_value_at(map, i);
		if(map->keys[i] >= 0 && r->style != key && (r->style & ~R_STATE_MASK) == key) map->keys[i] = -2;
	}
	make(t, key, false);
}

static void make_states(theme_state* t, int key, int states)
{
	int declared = 0;
	for(int i = 0; i < t->num_decls; i++)
	{
		theme_decl* d = &t->decls[i];
		if(!applies(d->key & ~R_STATE_MASK, key)) continue;
		declared |= d->key & states & R_STATE_MASK & (hover_only(d) ? ~R_STATE_HOVER : ~0);
	}
	bool hover = states & R_STATE_HOVER;
	if(hover) make(t, key, true);
	for(int s = declared; s; s = (s - 1) & declared) make(t, key | s, hover);
}

static int cur_key(r_draw* c){ theme_state* t = theme(c); assert(t->cur); return t->cur ? t->cur->key : 0; }

void r_theme_build(r_draw* c)
{
	int key = cur_key(c);
	if(key & R_STATE_MASK) make(c->theme, key, false);
	else make_base(c->theme, key);
}

void r_theme_build_states(r_draw* c, int states){ make_states(c->theme, cur_key(c) & ~R_STATE_MASK, states); }

void r_theme_build_joined(r_draw* c, int states)
{
	static const int comps[] = { R_COMPONENT_ENDCAP_START, R_COMPONENT_JOINED, R_COMPONENT_ENDCAP_END };
	int key = cur_key(c) & ~R_STATE_MASK & ~R_COMPONENT_MASK;
	for(int i = 0; i < COUNTOF(comps); i++)
	{
		make_base(c->theme, key | comps[i]);
		make_states(c->theme, key | comps[i], states);
	}
}

static void raised(r_draw* c)
{
	const r_draw_palette* p = &c->palette;
	r_theme_fill(c, p->control, 0);
	r_theme_border(c, 1, p->border);
	r_theme_shadow(c, 1, 1, dl(p, 0.6f, 0.2f));
}

static void selection_and_caret(r_draw* c, int key)
{
	const r_draw_palette* p = &c->palette;
	for(int caret = 0; caret < 2; caret++)
	{
		r_theme(c, key | (caret ? R_COMPONENT_CARET : R_COMPONENT_SELECTION));
		r_theme_fill(c, caret ? p->focus : alpha(p->selection, dl(p, 0.45f, 0.28f)), 0);
		r_theme_border(c, 0, 0);
		r_theme_surface(c, p->input);
		r_theme_radius(c, caret ? 0 : 2);
		r_theme_padding(c, 0, 0);
		r_theme_label(c, p->text, 0);
		r_theme_build(c);
	}
}

static void field(r_draw* c, int key, r_alignment align)
{
	const r_draw_palette* p = &c->palette;

	r_theme(c, key | R_STATE_FOCUS);
	r_theme_border_color(c, p->focus);
	r_theme_glow(c, alpha(p->focus, dl(p, 0.35f, 0.25f)), 2);

	r_theme(c, key);
	r_theme_fill(c, p->input, 0);
	r_theme_border(c, 1, p->border_strong);
	r_theme_radius(c, 2);
	r_theme_padding(c, 6, 2);
	r_theme_label(c, p->text_strong, align);
}

static void text_field(r_draw* c, int key, r_alignment align)
{
	field(c, key, align);
	r_theme_build(c);
	r_theme_build_states(c, R_STATE_HOVER | R_STATE_FOCUS);

	r_theme(c, key | R_VARIANT_TERTIARY);
	r_theme_label_color_auto(c, c->palette.text_dim);
	r_theme_build(c);

	selection_and_caret(c, key);
}

static uint32_t fade(uint32_t color, float a) { return color ? alpha(color, (color >> 24) / 255.0f * a) : 0; }

static int faded_box(theme_state* t, int slot, float a)
{
	if(slot <= 0 || slot >= RCUT_DRAW_USER_STYLEBOXES) return slot;
	r_draw_stylebox_def d;
	memcpy(&d, &t->c->styleboxes[slot].definition, sizeof(d));
	d.transparency = 1.0f - (1.0f - d.transparency) * a;
	return add_box(t, &d);
}

static void disabled_rules(theme_state* t, float a)
{
	r_hashmap* map = &t->c->style_rules;
	static r_draw_style_rule rules[256];
	int n = 0;
	for(int i = 0; i < map->num_slots && n < COUNTOF(rules); i++)
	{
		r_draw_style_rule* r = hash_value_at(map, i);
		if(map->keys[i] >= 0 && !(r->style & (R_STATE_HOVER | R_STATE_PRESSED | R_STATE_FOCUS | R_STATE_DISABLED))
			&& !r_draw_get_style_rule(t->c, r->style | R_STATE_DISABLED)) rules[n++] = *r;
	}
	for(int i = 0; i < n; i++)
	{
		r_draw_style_rule r = rules[i];
		r.style |= R_STATE_DISABLED;
		r.bg_stylebox = faded_box(t, r.bg_stylebox, a);
		r.overlay     = faded_box(t, r.overlay, a);
		r.label_color = fade(r.label_color, a);
		r.icon_color  = fade(r.icon_color, a);
		r.text_outline_color = fade(r.text_outline_color, a);
		r.text_shadow_color  = fade(r.text_shadow_color, a);
		r.hover_overlay = r.focus_overlay = 0;
		r_draw_set_style_rule(t->c, r.style, &r);
	}
}

void __draw_theme_flush(r_draw* c)
{
	theme_state* t = c->theme;
	if(!t->dirty) return;
	t->dirty = false;
	disabled_rules(t, 0.4f);
}

void r_theme_default(r_draw* c, const r_draw_theme_options* o)
{
	const r_draw_theme_options* d = &r_draw_default_theme_options;
	if(!o) o = d;
	uint32_t primary = o->primary ? o->primary : d->primary;
	#define ACCENT(name) .name = o->name ? o->name : primary
	r_draw_theme_options resolved = {
		.background = o->background ? o->background : d->background,
		.text       = o->text,
		.primary    = primary, ACCENT(checked), ACCENT(highlight), ACCENT(value), ACCENT(selection), ACCENT(focus),
		.font_size  = o->font_size > 0 ? o->font_size : d->font_size,
		.scrollbar_buttons = o->scrollbar_buttons,
	};
	#undef ACCENT
	const r_draw_theme_options* options = &resolved;

	for(int i = 0; i < R_ICON_USER; i++)
	if(default_icons[i])
		c->icons[i].ops = default_icons[i];

	hash_clear(&c->style_rules);
	for(int i = 1; i < RCUT_DRAW_USER_STYLEBOXES; i++)
		r_draw_free_stylebox(c, i);
	r_draw_clear_cache(c);
	for(r_ui_list* l = c->context->chain_start; l; l = l->chain_next)
		l->needs_redraw = true;

	if(!c->theme) c->theme = malloc(sizeof(theme_state));
	theme_state* t = c->theme;
	memset(t, 0, sizeof(*t));
	t->c = c; t->next_slot = 1;
	c->palette = make_palette(options);
	const r_draw_palette* p = &c->palette;
	int fs = (int)(options->font_size + 0.5f);
	t->control_h = fs + 18;
	t->custom_text = options->text != 0;
	t->font   = r_draw_get_font(c, 0, fs, R_FONT_SHAPED);
	t->bold   = r_draw_get_font(c, 0, fs, R_FONT_BOLD | R_FONT_SHAPED);
	t->small  = r_draw_get_font(c, 0, fs - 1, R_FONT_SHAPED);
	t->header = r_draw_get_font(c, 0, (fs * 9 + 3) / 7, R_FONT_BOLD | R_FONT_SHAPED);
	c->clear_color = p->panel;
	uint32_t checked = deepen(p->checked, 0.15f), value = deepen(p->value, 0.15f);

	t->focus_ring = alpha(p->focus, dl(p, 0.75f, 0.55f));

	r_theme(c, R_STYLE_FALLBACK);
	r_theme_font(c, t->font);
	r_theme_label(c, p->text, R_LEFT_CENTER);
	r_theme_layout(c, t->control_h, 8);
	r_theme_build(c);

	r_theme(c, R_STYLE_FALLBACK | R_STATE_HOVER);
	r_theme_hover(c, p->hover_overlay);

	r_theme(c, R_STYLE_FALLBACK | R_STATE_PRESSED);
	r_theme_no_shadow(c);
	r_theme_inset(c, 3, 10, dl(p, 0.7f, 0.35f));

	r_theme(c, R_STYLE_FALLBACK | R_STATE_TOGGLED);
	r_theme_fill(c, checked, 0);
	r_theme_border_color(c, checked);
	r_theme_label_color_auto(c, p->accent_text);

	r_theme(c, R_STYLE_IMAGE);
	r_theme_build(c);

	// Text
	r_theme(c, R_STYLE_TEXT);
	r_theme_build(c);

	r_theme(c, R_STYLE_LABEL);
	r_theme_build(c);

	r_theme(c, R_STYLE_LABEL | R_VARIANT_TERTIARY);
	r_theme_label_color_auto(c, p->text_dim);
	r_theme_build(c);

	r_theme(c, R_STYLE_LABEL | R_VARIANT_PRIMARY);
	r_theme_font(c, t->bold);
	r_theme_label_color_auto(c, p->text_strong);
	r_theme_build(c);

	r_theme(c, R_STYLE_HEADER);
	r_theme_font(c, t->header);
	r_theme_label_color_auto(c, p->text_strong);
	r_theme_build(c);

	r_theme(c, R_STYLE_ICON);
	r_theme_label(c, p->text, R_CENTER);
	r_theme_layout(c, 16, 0);
	r_theme_build(c);

	r_theme(c, R_STYLE_PANEL);
	r_theme_fill(c, p->view, 0);
	r_theme_border(c, 1, p->border);
	r_theme_radius(c, 4);
	r_theme_padding(c, 4, 4);
	r_theme_build(c);

	r_theme(c, R_STYLE_PANEL | R_VARIANT_RAISED);
	r_theme_fill(c, p->card, 0);
	r_theme_radius(c, 4);
	r_theme_padding(c, 14, 12);
	r_theme_shadow(c, 1, 3, dl(p, 0.35f, 0.1f));
	r_theme_build(c);

	r_theme(c, R_STYLE_WINDOW);
	r_theme_fill(c, p->window, 0);
	r_theme_build(c);

	r_theme(c, R_STYLE_SEPARATOR | R_STATE_HOVER);
	r_theme_fill(c, p->focus, 0);

	r_theme(c, R_STYLE_SEPARATOR);
	r_theme_fill(c, p->border, 0);
	r_theme_build(c);
	r_theme_build_states(c, R_STATE_HOVER);

	int button_states = R_STATE_HOVER | R_STATE_PRESSED;
	int joined[] = { R_COMPONENT_ENDCAP_START, R_COMPONENT_JOINED, R_COMPONENT_ENDCAP_END };
	for(int s = 0; s < 2; s++)
	{
		int style = s ? R_STYLE_SEGMENTED : R_STYLE_BUTTON;
		for(int i = 0; i < 3; i++)
		{
			r_theme(c, style | joined[i]);
			r_theme_padding(c, 0, 4);
			r_theme_no_shadow(c);
			r_theme_focus_border(c, p->focus);

			r_theme(c, style | joined[i] | R_STATE_TOGGLED);
			r_theme_focus_border(c, alpha(p->accent_text, 0.8f));
		}

		r_theme(c, style | R_COMPONENT_TRACK);
		r_theme_fill(c, 0, 0);
		r_theme_border(c, 0, 0);
		r_theme_label_color_auto(c, 0);
		r_theme_focus(c, R_FOCUS_NONE);
		r_theme_build(c);
	}

	r_theme(c, R_STYLE_BUTTON);
	raised(c);
	r_theme_radius(c, 6);
	r_theme_padding(c, 14, 4);
	r_theme_label(c, p->text_strong, R_CENTER);
	r_theme_focus(c, R_FOCUS_RING);
	r_theme_build(c);
	r_theme_build_states(c, button_states);
	r_theme_build_joined(c, button_states | R_STATE_TOGGLED);

	r_theme(c, R_STYLE_SEGMENTED);
	raised(c);
	r_theme_radius(c, 3);
	r_theme_label(c, p->text, R_CENTER);
	r_theme_focus(c, R_FOCUS_RING);
	r_theme_build(c);
	r_theme_build_joined(c, button_states | R_STATE_TOGGLED);

	r_theme(c, R_STYLE_BUTTON | R_VARIANT_PRIMARY);
	r_theme_fill(c, p->primary, deepen(p->primary, 0.12f));
	r_theme_border_color(c, deepen(p->primary, 0.06f));
	r_theme_label_color_auto(c, p->accent_text);
	r_theme_build(c);
	r_theme_build_states(c, button_states);

	r_theme(c, R_STYLE_BUTTON | R_VARIANT_TERTIARY);
	r_theme_fill(c, alpha(p->control, 0.5f), 0);
	r_theme_border_color(c, alpha(p->border, 0.5f));
	r_theme_no_shadow(c);
	r_theme_label_color_auto(c, p->text);
	r_theme_build(c);
	r_theme_build_states(c, button_states);

	r_theme(c, R_STYLE_DROPDOWN | R_STATE_TOGGLED);
	r_theme_fill(c, p->control_pressed, 0);
	r_theme_border_color(c, p->border);
	r_theme_no_shadow(c);
	r_theme_label_color_auto(c, p->text_strong);
	r_theme_icon(c, p->text, 0);
	r_theme_focus(c, R_FOCUS_NONE);

	r_theme(c, R_STYLE_DROPDOWN);
	raised(c);
	r_theme_radius(c, 3);
	r_theme_padding(c, 10, 4);
	r_theme_label(c, p->text_strong, R_LEFT_CENTER);
	r_theme_icon(c, p->text_dim, 0);
	r_theme_focus(c, R_FOCUS_RING);
	r_theme_build(c);
	r_theme_build_states(c, button_states | R_STATE_TOGGLED);

	text_field(c, R_STYLE_INPUT, R_LEFT_CENTER);
	text_field(c, R_STYLE_INPUT_MULTILINE_EDIT, R_TOP_LEFT);

	field(c, R_STYLE_SPINBOX, R_LEFT_CENTER);
	r_theme_corners(c, 3, 0, 3, 0);
	r_theme_build(c);
	r_theme(c, R_STYLE_SPINBOX | R_STATE_FOCUS);
	r_theme_border_color(c, c->palette.focus);
	r_theme_build_states(c, R_STATE_HOVER | R_STATE_FOCUS);

	selection_and_caret(c, R_STYLE_SPINBOX);

	field(c, R_STYLE_SLIDEBOX, R_CENTER);
	r_theme_focus(c, R_FOCUS_RING);
	r_theme_build(c);
	r_theme_build_states(c, R_STATE_HOVER);

	r_theme(c, R_STYLE_SLIDEBOX | R_COMPONENT_FILL);
	r_theme_fill(c, dlc(p, deepen(value, 0.3f), over_straight(value, alpha(WHITE, 0.5f))), 0);
	r_theme_border(c, 0, 0);
	r_theme_radius(c, 0);
	r_theme_padding(c, 1, 1);
	r_theme_label(c, p->text, 0);
	r_theme_focus(c, R_FOCUS_NONE);
	r_theme_build(c);

	for(int round = 0; round < 2; round++)
	{
		r_theme(c, round ? R_STYLE_RADIO : R_STYLE_CHECKBOX);
		r_theme_fill(c, p->input, 0);
		r_theme_border(c, 1, p->border_hover);
		r_theme_radius(c, round ? 9 : 2);
		r_theme_label(c, p->text, R_CENTER);
		r_theme_icon(c, p->accent_text, round ? 8 : 14);
		r_theme_focus(c, R_FOCUS_RING);
		r_theme_layout(c, 18, 0);
		r_theme_build(c);
		r_theme_build_states(c, R_STATE_HOVER | R_STATE_TOGGLED);

		r_theme(c, (round ? R_STYLE_RADIO : R_STYLE_CHECKBOX) | R_VARIANT_FLAT);
		r_theme_border(c, 1, p->border_hover);

		r_theme(c, (round ? R_STYLE_RADIO : R_STYLE_CHECKBOX) | R_VARIANT_FLAT | R_STATE_TOGGLED);
		r_theme_fill(c, 0, 0);
		r_theme_icon(c, checked, round ? 8 : 14);
	}

	r_theme(c, R_STYLE_TOGGLE | R_COMPONENT_TRACK);
	r_theme_fill(c, p->track, 0);
	r_theme_border(c, 1, p->border_strong);
	r_theme_radius(c, 9);
	r_theme_focus(c, R_FOCUS_RING);
	r_theme_layout(c, 18, 0);
	r_theme_build(c);
	r_theme_build_states(c, R_STATE_HOVER | R_STATE_TOGGLED);

	r_theme(c, R_STYLE_TOGGLE | R_COMPONENT_THUMB);
	r_theme_fill(c, p->thumb, 0);
	r_theme_radius(c, 7);
	r_theme_shadow(c, 1, 2, dl(p, 0.5f, 0.25f));
	r_theme_layout(c, 16, 0);
	r_theme_build(c);

	r_theme(c, R_STYLE_SLIDER | R_COMPONENT_TRACK);
	r_theme_fill(c, p->track, 0);
	r_theme_radius(c, 3);
	r_theme_build(c);

	r_theme(c, R_STYLE_SLIDER | R_COMPONENT_FILL);
	r_theme_fill(c, value, 0);
	r_theme_radius(c, 3);
	r_theme_build(c);

	r_theme(c, R_STYLE_SLIDER | R_COMPONENT_THUMB | R_STATE_HOVER);
	r_theme_glow(c, alpha(p->value, 0.35f), 3);

	r_theme(c, R_STYLE_SLIDER | R_COMPONENT_THUMB);
	r_theme_fill(c, p->thumb, 0);
	r_theme_radius(c, 8);
	r_theme_shadow(c, 1, 3, dl(p, 0.55f, 0.2f));
	r_theme_focus(c, R_FOCUS_RING);
	r_theme_layout(c, 16, 0);
	r_theme_build(c);
	r_theme_build_states(c, R_STATE_HOVER);

	r_theme(c, R_STYLE_PROGRESS | R_COMPONENT_TRACK);
	r_theme_fill(c, p->track, 0);
	r_theme_radius(c, 3);
	r_theme_build(c);

	r_theme(c, R_STYLE_PROGRESS | R_COMPONENT_FILL);
	r_theme_fill(c, value, 0);
	r_theme_radius(c, 3);
	r_theme_build(c);

	r_theme(c, R_STYLE_SCROLLBAR | R_COMPONENT_TRACK);
	r_theme_layout(c, 10, 0);
	r_theme_build(c);

	r_theme(c, R_STYLE_SCROLLBAR | R_COMPONENT_THUMB | R_STATE_HOVER);
	r_theme_fill(c, p->border_hover, 0);

	r_theme(c, R_STYLE_SCROLLBAR | R_COMPONENT_THUMB | R_STATE_PRESSED);
	r_theme_fill(c, p->text_dim, 0);
	r_theme_inset(c, 0, 0, 0);

	r_theme(c, R_STYLE_SCROLLBAR | R_COMPONENT_THUMB);
	r_theme_fill(c, p->border_strong, 0);
	r_theme_radius(c, 2);
	r_theme_build(c);
	r_theme_build_states(c, button_states);

	for(int end = 0; end < 2; end++)
	{
		r_theme(c, R_STYLE_SCROLLBAR | (end ? R_COMPONENT_ENDCAP_END : R_COMPONENT_ENDCAP_START));
		r_theme_layout(c, options->scrollbar_buttons ? 12 : -1, 0);
		r_theme_radius(c, 2);
		r_theme_label(c, p->text_dim, R_CENTER);
		r_theme_icon(c, 0, 10);
		r_theme_build(c);
		r_theme_build_states(c, button_states);
	}

	for(int context = 0; context < 2; context++)
	{
		r_theme(c, context ? R_STYLE_CONTEXT_MENU_BG : R_STYLE_MENU_BG);
		r_theme_fill(c, p->menu, 0);
		r_theme_border(c, 1, dlc(p, p->border_strong, p->border));
		r_theme_build(c);
	}

	int menu_states = R_STATE_HOVER | R_STATE_FOCUS | R_STATE_SELECTED;

	r_theme(c, R_STYLE_MENU_ITEM | R_STATE_SELECTED);
	r_theme_fill(c, alpha(p->highlight, dl(p, 0.22f, 0.14f)), 0);
	r_theme_icon(c, 0, 0);

	r_theme(c, R_STYLE_MENU_ITEM | R_VARIANT_TERTIARY | R_STATE_SELECTED);
	r_theme_fill(c, 0, 0);

	for(int i = 0; i < 2; i++)
	{
		int state = i ? R_STATE_FOCUS : R_STATE_HOVER;

		r_theme(c, R_STYLE_MENU_ITEM | state);
		r_theme_fill(c, p->highlight, 0);
		r_theme_label_color_auto(c, p->accent_text);
		r_theme_icon(c, 0, 0);

		r_theme(c, R_STYLE_MENU_ITEM | R_VARIANT_TERTIARY | state);
		r_theme_label_color_auto(c, alpha(p->accent_text, 0.8f));
	}

	r_theme(c, R_STYLE_MENU_ITEM);
	r_theme_surface(c, p->menu);
	r_theme_padding(c, 8, 3);
	r_theme_label(c, p->text_strong, R_LEFT_CENTER);
	r_theme_icon(c, p->text_dim, 0);
	r_theme_build(c);
	r_theme_build_states(c, menu_states);

	r_theme(c, R_STYLE_MENU_ITEM | R_VARIANT_TERTIARY);
	r_theme_font(c, t->small);
	r_theme_label(c, p->text_dim, R_RIGHT_CENTER);
	r_theme_icon(c, 0, 0);
	r_theme_build(c);
	r_theme_build_states(c, menu_states);

	r_theme(c, R_STYLE_MENUBAR_BG);
	r_theme_fill(c, p->panel, 0);
	r_theme_underline(c, 1, p->border);
	r_theme_build(c);

	r_theme(c, R_STYLE_MENUBAR_ITEM | R_STATE_HOVER);
	r_theme_label_color_auto(c, p->text_strong);

	r_theme(c, R_STYLE_MENUBAR_ITEM | R_STATE_SELECTED);
	r_theme_fill(c, p->control_hover, 0);
	r_theme_label_color_auto(c, p->text_strong);

	r_theme(c, R_STYLE_MENUBAR_ITEM | R_STATE_FOCUS);
	r_theme_fill(c, p->control_hover, 0);
	r_theme_label_color_auto(c, p->text_strong);
	r_theme_focus(c, R_FOCUS_RING);

	r_theme(c, R_STYLE_MENUBAR_ITEM);
	r_theme_padding(c, 10, 2);
	r_theme_label(c, p->text, R_CENTER);
	r_theme_build(c);
	r_theme_build_states(c, menu_states);

	r_theme(c, R_STYLE_TOOLTIP);
	r_theme_fill(c, p->tooltip, 0);
	r_theme_border(c, 1, dlc(p, p->border_strong, p->tooltip));
	r_theme_padding(c, 9, 5);
	r_theme_font(c, t->small);
	r_theme_label_color_auto(c, p->text_strong);
	r_theme_build(c);

	r_theme(c, R_STYLE_TAB_BAR);
	r_theme_build(c);

	r_theme(c, R_STYLE_TAB | R_STATE_HOVER);
	r_theme_label_color_auto(c, p->text);

	r_theme(c, R_STYLE_TAB | R_STATE_SELECTED);
	r_theme_fill(c, p->view, 0);
	r_theme_bevel(c, 0, p->view);
	r_theme_font(c, t->bold);
	r_theme_label_color_auto(c, p->text_strong);

	r_theme(c, R_STYLE_TAB);
	r_theme_fill(c, p->control, 0);
	r_theme_border(c, 1, p->border);
	r_theme_padding(c, 12, 0);
	r_theme_label(c, p->text_dim, R_CENTER);
	r_theme_focus(c, R_FOCUS_RING);
	r_theme_build(c);
	r_theme_build_states(c, R_STATE_HOVER | R_STATE_SELECTED);

	r_theme(c, R_STYLE_TAB_PAGE);
	r_theme_fill(c, p->view, 0);
	r_theme_border(c, 1, p->border);
	r_theme_corners(c, 0, 0, 4, 4);
	r_theme_padding(c, 4, 4);
	r_theme_build(c);

	r_theme(c, R_STYLE_LIST_ITEM | R_STATE_HOVER);
	r_theme_label_color_auto(c, p->text_strong);

	r_theme(c, R_STYLE_LIST_ITEM | R_STATE_SELECTED);
	r_theme_fill(c, p->selection, 0);
	r_theme_label_color_auto(c, p->accent_text);

	r_theme(c, R_STYLE_LIST_ITEM | R_STATE_SELECTED | R_STATE_HOVER);
	r_theme_label_color_auto(c, p->accent_text);

	r_theme(c, R_STYLE_LIST_ITEM);
	r_theme_surface(c, p->view);
	r_theme_padding(c, 10, 0);
	r_theme_focus(c, R_FOCUS_RING);
	r_theme_build(c);
	r_theme_build_states(c, R_STATE_HOVER | R_STATE_SELECTED);

	r_theme(c, R_STYLE_COLLAPSIBLE | R_STATE_HOVER);
	r_theme_icon(c, p->text, 0);

	r_theme(c, R_STYLE_COLLAPSIBLE);
	r_theme_padding(c, 6, 0);
	r_theme_font(c, t->bold);
	r_theme_label_color_auto(c, p->text_strong);
	r_theme_icon(c, p->text_dim, 0);
	r_theme_focus(c, R_FOCUS_RING);
	r_theme_build(c);
	r_theme_build_states(c, R_STATE_HOVER);

	r_theme(c, R_STYLE_BACKDROP);
	r_theme_fill(c, dlc(p, alpha(BLACK, 0.55f), alpha(p->text_strong, 0.3f)), 0);
	r_theme_build(c);

	r_theme(c, R_STYLE_DIALOG);
	r_theme_fill(c, dlc(p, p->menu, p->card), 0);
	r_theme_border(c, 1, dlc(p, p->border_strong, p->border));
	r_theme_radius(c, 4);
	r_theme_padding(c, 20, 16);
	r_theme_shadow(c, 1, 3, dl(p, 0.6f, 0.25f));
	r_theme_build(c);

	r_theme(c, R_VARIANT_FLAT);
	r_theme_fill(c, 0, 0);
	r_theme_border(c, 0, 0);
	r_theme_no_shadow(c);

	r_theme(c, R_FLAG_EMBEDDED);
	r_theme_fill(c, 0, 0);
	r_theme_border(c, 0, 0);
	r_theme_no_shadow(c);

	static const struct { int style, states; } flat[] = {
		{ R_STYLE_BUTTON, R_STATE_HOVER | R_STATE_PRESSED }, { R_STYLE_DROPDOWN, R_STATE_HOVER | R_STATE_PRESSED | R_STATE_TOGGLED },
		{ R_STYLE_SEGMENTED, 0 }, { R_STYLE_INPUT, R_STATE_HOVER | R_STATE_FOCUS }, { R_STYLE_INPUT_MULTILINE_EDIT, R_STATE_HOVER | R_STATE_FOCUS },
		{ R_STYLE_SPINBOX, R_STATE_HOVER | R_STATE_FOCUS }, { R_STYLE_SLIDEBOX, R_STATE_HOVER }, { R_STYLE_CHECKBOX, R_STATE_HOVER | R_STATE_TOGGLED },
		{ R_STYLE_RADIO, R_STATE_HOVER | R_STATE_TOGGLED }, { R_STYLE_TAB, R_STATE_HOVER | R_STATE_SELECTED }, { R_STYLE_TAB_PAGE, 0 },
		{ R_STYLE_PANEL, 0 }, { R_STYLE_DIALOG, 0 }, { R_STYLE_TOOLTIP, 0 }, { R_STYLE_MENU_BG, 0 }, { R_STYLE_CONTEXT_MENU_BG, 0 },
		{ R_STYLE_MENUBAR_BG, 0 },
	};
	for(int i = 0; i < COUNTOF(flat); i++)
	{
		r_theme_tweak(c, flat[i].style | R_VARIANT_FLAT);
		r_theme_build(c);
		r_theme_build_states(c, flat[i].states);
	}

	int embedded[] = { R_STYLE_BUTTON, R_STYLE_DROPDOWN };
	for(int i = 0; i < COUNTOF(embedded); i++)
	{
		r_theme_tweak(c, embedded[i] | R_FLAG_EMBEDDED);
		r_theme_build(c);
		r_theme_build_states(c, R_STATE_HOVER | R_STATE_PRESSED | R_STATE_TOGGLED);
	}
}
