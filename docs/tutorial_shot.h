// Force-included into the tutorial examples by the docs target: saves the first rendered frame to SHOT and exits.
#include "rcut_sdl.h"
#define r_sdl_init(...) (SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen"), r_sdl_init(__VA_ARGS__))
#define r_sdl_render_ui(f) (r_sdl_screenshot(f, SHOT), exit(0))
