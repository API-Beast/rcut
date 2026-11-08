#include <SDL3/SDL.h>
#include "rcut_sdl.h"

static bool quit;

static void on_ok(void)
{
	quit = true;
}

int main(void)
{
	r_sdl_frontend* f = r_sdl_init("rcut tutorial", 320, 150, 0);
	if(!f) return 1;
	r_ui_list* ui = f->root;

	rect ok = { 110, 75, 100, 50, .id = 2 };
	r_label(ui, (rect){ 63, 0, 194, 75, .id = 1 }, "Hello World from rcut! 😎👍");
	r_button(ui, ok, "OK");
	r_on_click(ui, ok, on_ok);

	while(!quit && !r_sdl_quit_requested(f, true))
	{
		SDL_Event e;

		SDL_WaitEventTimeout(NULL, 5); // Wait up to 5ms for an event. This is optional: it simply reduces battery usage.
		while(SDL_PollEvent(&e))
			r_sdl_process_event(f, &e);

		r_sdl_update(f);
		r_sdl_render_ui(f);
	}
	r_sdl_free(f);
	return 0;
}
