#include <stdio.h>
#include "rcut_sdl.h"

static void on_ok(void) {}
static void on_select(int32_t index) { (void)index; }
static void on_close(void) {}

//@concept rect Rects
//@concept salting Salting
//@concept alignment Alignment
//@concept layout_iter Layout iterators
//@concept rich_text Rich text
//@concept ui_list UI lists
//@concept rebuild Rebuilding
//@concept target Targets
//@concept ephemeral Ephemerals
//@concept drawable Drawables
//@concept interactable Interactables
//@concept binding Data binding
//@concept dynamic_rect Dynamic rects
//@concept callback Callbacks
//@concept menu Menus
//@concept storage Storage
//@concept context Context
//@concept frontend Frontends
//@concept damage Damage
//@concept input Input
//@concept draw-context Draw context
//@concept draw-function Draw functions
//@concept draw_backend Draw backend
//@concept dpi_scale DPI scale
//@concept colors Colors
//@concept theming Theming
//@concept style_rule Style rules
//@concept default_theme Default theme
//@concept stylebox Styleboxes
//@concept font Fonts
//@concept icon Icons
//@concept image Image buffers

#include "api_documentation_core_basic.c"
#include "api_documentation_core_extended.c"
#include "api_documentation_draw.c"
#include "api_documentation_sdl.c"

#define DOC(fn, w, h) { #fn, fn, w, h }
static const struct { const char* name; void (*fn)(r_sdl_frontend*, r_ui_list*); int w, h; } docs[] = {
	DOC(doc_r_create_ui_list, 320, 80),
	DOC(doc_r_salt, 240, 60),
	DOC(doc_r_cut_px, 360, 200),
	DOC(doc_r_stack, 200, 140),
	DOC(doc_r_next, 320, 60),
	DOC(doc_r_text, 260, 80),
	DOC(doc_r_button, 240, 80),
	DOC(doc_r_label, 240, 80),
	DOC(doc_r_checkbox, 200, 80),
	DOC(doc_r_icon, 200, 48),
	DOC(doc_r_header, 220, 80),
	DOC(doc_r_panel, 260, 232),
	DOC(doc_r_separator, 200, 90),
	DOC(doc_r_toggle, 200, 80),
	DOC(doc_r_radio, 300, 52),
	DOC(doc_r_selectable, 220, 112),
	DOC(doc_r_slider_h, 260, 80),
	DOC(doc_r_slider_v, 140, 140),
	DOC(doc_r_slider_box, 260, 56),
	DOC(doc_r_spinbox, 200, 60),
	DOC(doc_r_progress, 240, 70),
	DOC(doc_r_tabs, 280, 140),
	DOC(doc_r_dropdown, 220, 152),
	DOC(doc_r_segmented, 260, 60),
	DOC(doc_r_tree_node, 220, 136),
	DOC(doc_r_split, 300, 120),
	DOC(doc_r_line_edit, 260, 100),
	DOC(doc_r_text_area, 260, 110),
	DOC(doc_r_text_edit_ex, 260, 56),
	DOC(doc_r_menubar, 260, 106),
	DOC(doc_r_image, 300, 90),
	DOC(doc_r_theme_default_crimson, 360, 232),
	DOC(doc_r_theme_default_navy, 360, 232),
	DOC(doc_r_theme_default_violet, 360, 232),
	DOC(doc_r_theme_default_green, 360, 232),
	DOC(doc_r_draw_upload_icon, 420, 56),
};

int main(int argc, char** argv)
{
	const char* dir = argc > 1 ? argv[1] : "docs/api/img";
	SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
	
	for(size_t i = 0; i < sizeof(docs) / sizeof(docs[0]); i++)
	{
		r_sdl_frontend* f = r_sdl_init(docs[i].name, docs[i].w, docs[i].h, R_SDL_HIDDEN);
		if(!f) return 1;
		f->no_popup_windows = true;
		docs[i].fn(f, f->root);
		r_sdl_update(f);
		char path[512];
		snprintf(path, sizeof(path), "%s/%s.png", dir, docs[i].name);
		if(!r_sdl_screenshot(f, path)) fprintf(stderr, "failed: %s\n", path);
		r_sdl_free(f);
	}
	return 0;
}
