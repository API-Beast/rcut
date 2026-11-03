#pragma once
#include "rcut_draw.h"
#include <SDL3/SDL.h>

#define RCUT_SDL_MAX_WINDOWS 128
#define RCUT_SDL_CACHE_PIXELS (2048 * 2048) // Software cache shared by all windows, it must hold everything a frame uses.

typedef struct r_sdl_window
{
	SDL_Window*  window;
	SDL_Surface* internal_surface;
	SDL_Surface* last_window_surface; // NULL if not drawn yet.
	int drawn_x, drawn_y, drawn_w, drawn_h;

	int domain;
	int parent;
	int x, y, w, h;
	
	bool used:1;
	bool close_requested:1;
	bool tooltip:1;
	bool hidden:1;
} r_sdl_window;

typedef struct r_sdl_frontend
{
	r_draw    draw;
	r_context context;
	r_draw_software software;

	r_sdl_window windows[RCUT_SDL_MAX_WINDOWS];
	r_ui_list* root;
	uint64_t last_tick_time;
	bool no_popup_windows;
	bool quit_requested;
	SDL_Window* text_input_window;
	bool auto_scale;
	bool vsync;
	char* clipboard;
	int   cursor;
	SDL_Cursor* cursors[8];
} r_sdl_frontend;

enum r_sdl_flags
{
	R_SDL_RESIZABLE = 1 << 0,
	R_SDL_VSYNC     = 1 << 1,
};

r_sdl_frontend* r_sdl_init(const char* title, int w, int h, int flags);
void r_sdl_free  (r_sdl_frontend* c);
bool r_sdl_process_event(r_sdl_frontend* c, SDL_Event* e);
bool r_sdl_quit_requested(r_sdl_frontend* c, bool clear);
r_ui_list* r_sdl_dialog(r_sdl_frontend* c, uint64_t root_id, int w, int h, const char* title);
bool r_sdl_close_requested(r_sdl_frontend* c, r_ui_list* dialog, bool clear);
void r_sdl_update(r_sdl_frontend* c);
void r_sdl_render_ui(r_sdl_frontend* c);
void r_sdl_set_scale(r_sdl_frontend* c, float scale);
bool r_sdl_screenshot(r_sdl_frontend* c, const char* path);
