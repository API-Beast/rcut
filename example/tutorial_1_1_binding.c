#include <SDL3/SDL.h>
#include "rcut_sdl.h"
#include <stdio.h>
#include <math.h>

static r_sdl_frontend* f;
static float    brightness = 0.5f;
static float    db = -6;
static char     db_text[32] = "Volume: -6.0 dB";
static uint32_t flags = 1;

static float get_gain(void)
{
	return db <= -60 ? 0 : powf(10, db / 20);
}

static void set_gain(float gain)
{
	db = gain > 0.001f ? 20 * log10f(gain) : -60;
	snprintf(db_text, sizeof(db_text), "Volume: %.1f dB", db);
	r_damage_id(&f->context, 1);
}

int main(void)
{
	f = r_sdl_init("rcut tutorial", 320, 170, 0);
	if(!f) return 1;
	r_ui_list* ui = f->root;

	r_label (ui, (rect){ 16, 16, 288, 24, .id = 1 }, db_text);
	r_slider(ui, (rect){ 16, 44, 288, 24, .id = 2 }, 0, 1, 0.01f, R_GETSET(.getter_f32 = get_gain, .setter_f32 = set_gain));
	r_slider_box(ui, (rect){ 16, 84, 288, 28, .id = 3 }, "Brightness", 0, 1, 0.01f, R_PTR(&brightness));
	r_checkbox(ui, (rect){  16, 128, 140, 24, .id = 4 }, "Bold",   R_BITFIELD(&flags, 0, 1));
	r_checkbox(ui, (rect){ 164, 128, 140, 24, .id = 5 }, "Italic", R_BITFIELD(&flags, 1, 1));

	while(!r_sdl_quit_requested(f, true))
	{
		SDL_Event e;
		if(SDL_WaitEventTimeout(&e, 50))
		{
			do r_sdl_process_event(f, &e); while(SDL_PollEvent(&e));
		}
		r_sdl_update(f);
		r_sdl_render_ui(f);
	}
	r_sdl_free(f);
	return 0;
}
