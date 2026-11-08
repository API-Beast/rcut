#include <SDL3/SDL.h>
#include "rcut_sdl.h"

static void build(r_ui_list* ui)
{
	r_list_clear(ui);
	rect main   = ui->root_area;
	rect menu   = r_cut_px(R_TOP,    &main, 32,  R_SALT00);
	rect status = r_cut_px(R_BOTTOM, &main, 24,  R_SALT01);
	rect side   = r_cut_px(R_LEFT,   &main, 140, R_SALT02);

	r_panel(ui, menu,   R_STYLE_PANEL);
	r_panel(ui, status, R_STYLE_PANEL);
	r_panel(ui, side,   R_STYLE_PANEL);
	r_panel(ui, main,   R_STYLE_PANEL);
	r_label(ui, r_shrink_sides(menu,   8, 0, 8, 0), "Menu");
	r_label(ui, r_shrink_sides(status, 8, 0, 8, 0), "Status");
	r_label(ui, r_shrink_sides(side,   8, 0, 8, 0), "Side");
	r_label(ui, r_shrink_sides(main,   8, 0, 8, 0), "Main");
}

int main(void)
{
	r_sdl_frontend* f = r_sdl_init("rcut tutorial", 480, 300, R_SDL_RESIZABLE);
	if(!f) return 1;
	r_ui_list* ui = f->root;

	while(!r_sdl_quit_requested(f, true))
	{
		SDL_Event e;
		SDL_WaitEventTimeout(NULL, 5); // Wait up to 5ms for an event. This is optional: it simply reduces battery usage.
		while(SDL_PollEvent(&e))
			r_sdl_process_event(f, &e);

		if(ui->resized) build(ui);
		r_sdl_update(f);
		r_sdl_render_ui(f);
	}
	r_sdl_free(f);
	return 0;
}
