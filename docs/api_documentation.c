#include <stdio.h>
#include "rcut_sdl.h"

static bool  mute = true;
static float volume = 0.6f;
static bool  fullscreen = false;
static void on_ok(void) {}
static void on_select(int32_t index) { (void)index; }
static void on_close(void) {}

//@module core

//@section UI Lists
//! <TODO: >

//! Creates a new UI list, it continues to exist until deleted by @ref r_delete_ui_list.
//! 
//! UI lists store everything in rcut, in particular @ref r_interactables and @ref r_drawables.
//! Each UI list can be cleared and regenerated independently of each other.
//! 
//! `target` is a opaque target ID, usually referring to a window or render target. All coordinates are within that targets space.
//! See <TODO: rcut_SDL targets>.
//!
//! `r_create_ui_list_ex` takes explicit capacities instead of `RCUT_LIST_MAX_ELEMENTS`.
//! @note Memory is only reserved, not commited, as such there is no reason to use the _ex variant unless you are working on systems with small address spaces or processors without a MMU.
//! @see_also r_delete_ui_list r_popup r_viewport r_scroll_begin
//@function r_create_ui_list

//@example screenshot
static void doc_r_create_ui_list(r_sdl_frontend* f, r_ui_list* ui)
{
	r_ui_list* l = r_create_ui_list_ex(&f->context, 0, 100, 320, 80, 16, 4);
	r_label(l, l->root_area, "Hello from a second list");
}

//! <TODO: >
//! @param list <TODO: >
//! @see_also r_create_ui_list r_list_set_visibility
//@function r_destroy_ui_list

//@example
static void doc_r_destroy_ui_list(r_sdl_frontend* f, r_ui_list* ui)
{
	r_ui_list* dialog = r_sdl_dialog(f, 200, 320, 160, "Settings");
	if(r_sdl_close_requested(f, dialog, true))
		r_destroy_ui_list(dialog);
}

//! <TODO: >
//@function r_list_clear

//@example
static void doc_r_list_clear(r_sdl_frontend* f, r_ui_list* ui)
{
	if(ui->resized)
	{
		r_list_clear(ui);
		r_button(ui, r_shrink(ui->root_area, 16), "Rebuilt");
	}
}

//! <TODO: >
//@function r_list_set_visibility

//! <TODO: >
//! @see_also r_create_ui_list r_list_clear
//@struct r_ui_list

//! <TODO: >
//@define RCUT_LIST_MAX_ELEMENTS

//@section Rects and IDs
//! <TODO: >

//! <TODO: >
//@struct rect

//! <TODO: >
//! @see_also r_cut_px
//@function r_salt

//@example screenshot
static void doc_r_salt(r_sdl_frontend* f, r_ui_list* ui)
{
	rect area = r_shrink(ui->root_area, 12);
	rect size = { .w = 90, .h = 32 };
	rect yes  = r_aligned(R_LEFT_CENTER,  area, r_salt(size, R_SALT00));
	rect no   = r_aligned(R_RIGHT_CENTER, area, r_salt(size, R_SALT01));
	r_button(ui, yes, "Yes");
	r_button(ui, no,  "No");
}

//! <TODO: >
//! @param dir <TODO: >
//! @param to_cut <TODO: >
//! @param salt <TODO: >
//! @see_also r_salt r_stack
//@function r_cut_px

//! <TODO: >
//@example screenshot
static void doc_r_cut_px(r_sdl_frontend* f, r_ui_list* ui)
{
	rect main   = ui->root_area;
	rect menu   = r_cut_px(R_TOP,    &main, 32,  R_SALT00);
	rect status = r_cut_px(R_BOTTOM, &main, 24,  R_SALT01);
	rect side   = r_cut_px(R_LEFT,   &main, 100, R_SALT02);
	r_panel(ui, menu,   R_STYLE_PANEL);
	r_panel(ui, status, R_STYLE_PANEL);
	r_panel(ui, side,   R_STYLE_PANEL);
	r_label(ui, r_shrink(menu, 8),   "Menu");
	r_label(ui, r_shrink(status, 4), "Status");
	r_label(ui, r_shrink(side, 8),   "Side");
	r_label(ui, r_shrink(main, 8),   "Main");
}

//! <TODO: >
//@enum r_alignment

//@section Layout
//! <TODO: >

//! <TODO: >
//@struct r_layout_iter

//! <TODO: >
//! @see_also r_next r_remaining
//@function r_stack

//@example screenshot
static void doc_r_stack(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_button(ui, r_next(&it, -1), "New");
	r_button(ui, r_next(&it, -1), "Open");
	r_button(ui, r_next(&it, -1), "Save");
}

//! <TODO: >
//@function r_next

//! <TODO: >
//@example screenshot
static void doc_r_next(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter cols = r_stack(ui, r_shrink(ui->root_area, 12), R_LEFT);
	for(int i = 0; i < 3; i++)
		r_button(ui, r_next_fraction(&cols, 1.0f / (3 - i)), (char*[]){ "Low", "Medium", "High" }[i]);
}

//! <TODO: >
//@function r_remaining

//@section Widgets
//! <TODO: >

//! <TODO: >
//@function r_button

//@example screenshot
static void doc_r_button(r_sdl_frontend* f, r_ui_list* ui)
{
	rect ok = r_aligned(R_CENTER, ui->root_area, (rect){ .w = 100, .h = 32 });
	r_button(ui, ok, "OK");
	r_on_click(ui, ok, on_ok);
}

//! <TODO: >
//! @see_also r_paragraph r_header
//@function r_label

//@example screenshot
static void doc_r_label(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_label(ui, r_next(&it, 24), "Plain label");
	r_label_value(ui, r_next(&it, 24), "Volume: %.2f", R_PTR(&volume));
}

//! <TODO: >
//@function r_on_click

//@example
static void doc_r_on_click(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter it = r_stack(ui, ui->root_area, R_TOP);
	for(int i = 0; i < 4; i++)
	{
		rect r = r_next(&it, -1);
		r_button(ui, r, "Item");
		r_on_click_i32(ui, r, on_select, i);
	}
}

//! <TODO: >
//! @param binding <TODO: >
//@function r_checkbox

//@example screenshot
static void doc_r_checkbox(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_checkbox(ui, r_next(&it, -1), "Mute", R_PTR(&mute));
	r_checkbox(ui, r_next(&it, -1), "Fullscreen", R_PTR(&fullscreen));
}

//! <TODO: >
//! @see_also r_slider_box r_spinbox
//@function r_slider_h

//@example screenshot
static void doc_r_slider_h(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_label_value(ui, r_next(&it, 24), "Volume %.2f", R_PTR(&volume));
	r_slider_h(ui, r_next(&it, 24), 0, 1, 0.05f, R_PTR(&volume));
}

//! <TODO: >
//@enum r_style

//@section Data binding
//! <TODO: >

//! <TODO: >
//@struct r_data_binding

//! <TODO: >
//@define R_PTR

//@example
static void doc_R_PTR(r_sdl_frontend* f, r_ui_list* ui)
{
	r_checkbox(ui, ui->root_area, "Mute", R_PTR(&mute));
}

//@module sdl

//! <TODO: >
//! @param flags <TODO: >
//! @see_also r_sdl_free r_sdl_update
//@function r_sdl_init

//! <TODO: >
//@example
static void doc_r_sdl_init(r_sdl_frontend* f, r_ui_list* ui)
{
	r_sdl_frontend* app = r_sdl_init("Hello", 320, 200, R_SDL_RESIZABLE);
	while(!r_sdl_quit_requested(app, true))
	{
		SDL_Event e;
		if(SDL_WaitEventTimeout(&e, 50))
			do r_sdl_process_event(app, &e); while(SDL_PollEvent(&e));
		if(app->root->resized)
		{
			r_list_clear(app->root);
			r_label(app->root, app->root->root_area, "Hello");
		}
		r_sdl_update(app);
		r_sdl_render_ui(app);
	}
	r_sdl_free(app);
}

//! <TODO: >
//@enum r_sdl_flags

//! <TODO: >
//@struct r_sdl_frontend

//! <TODO: >
//@function r_sdl_update

//! <TODO: >
//@function r_sdl_render_ui

//! <TODO: >
//@function r_sdl_screenshot

#define DOC(fn, w, h) { #fn, fn, w, h }
static const struct { const char* name; void (*fn)(r_sdl_frontend*, r_ui_list*); int w, h; } docs[] = {
	DOC(doc_r_create_ui_list, 320, 80),
	DOC(doc_r_salt, 240, 60),
	DOC(doc_r_cut_px, 360, 200),
	DOC(doc_r_stack, 200, 140),
	DOC(doc_r_next, 320, 60),
	DOC(doc_r_button, 240, 80),
	DOC(doc_r_label, 240, 80),
	DOC(doc_r_checkbox, 200, 80),
	DOC(doc_r_slider_h, 260, 80),
};

int main(int argc, char** argv)
{
	const char* dir = argc > 1 ? argv[1] : "docs/api/img";
	for(size_t i = 0; i < sizeof(docs) / sizeof(docs[0]); i++)
	{
		r_sdl_frontend* f = r_sdl_init(docs[i].name, docs[i].w, docs[i].h, 0);
		if(!f) return 1;
		docs[i].fn(f, f->root);
		r_sdl_update(f);
		char path[512];
		snprintf(path, sizeof(path), "%s/%s.png", dir, docs[i].name);
		if(!r_sdl_screenshot(f, path)) fprintf(stderr, "failed: %s\n", path);
		r_sdl_free(f);
	}
	return 0;
}
