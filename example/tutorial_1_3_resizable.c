#include <SDL3/SDL.h>
#include "rcut_sdl.h"

static bool quit;

static struct { char* name; float db; bool mute; } channels[] = {
	{ "Master",  -6 },
	{ "Music",   -12 },
	{ "Voice",   0 },
	{ "Effects", -18, true },
};

static void on_ok(void)
{
	quit = true;
}

static void build(r_ui_list* ui)
{
	r_list_clear(ui);
	r_layout_iter window = r_stack(ui, r_shrink(ui->root_area, 16), R_BOTTOM);
	rect footer = r_next(&window, 32);
	r_layout_iter strips = r_stack(ui, r_remaining(&window), R_LEFT);
	for(int i = 0; i < 4; i++)
	{
		r_layout_iter strip = r_stack(ui, r_next_fraction(&strips, 1.0f / (4 - i)), R_TOP);
		r_label      (ui, r_next(&strip, 24), channels[i].name);
		r_checkbox   (ui, r_next(&strip, 24), "Mute", R_PTR(&channels[i].mute));
		r_label_value(ui, r_next(&strip, 24), "%.1f dB", R_PTR(&channels[i].db));
		r_slider_v   (ui, r_remaining(&strip), -60, 0, 0.1f, R_PTR(&channels[i].db));
	}
	rect ok = r_aligned(R_RIGHT, footer, (rect){ .w = 100, .h = 32 });
	r_button(ui, ok, "OK");
	r_on_click(ui, ok, on_ok);
}

int main(void)
{
	r_sdl_frontend* f = r_sdl_init("rcut tutorial", 400, 320, R_SDL_RESIZABLE);
	if(!f) return 1;
	r_ui_list* ui = f->root;

	while(!quit && !r_sdl_quit_requested(f, true))
	{
		SDL_Event e;
		if(SDL_WaitEventTimeout(&e, 50))
		{
			do r_sdl_process_event(f, &e); while(SDL_PollEvent(&e));
		}
		if(ui->resized) build(ui);
		r_sdl_update(f);
		r_sdl_render_ui(f);
	}
	r_sdl_free(f);
	return 0;
}
