//@module sdl

//! Creates the main window, the rcut context, a software renderer and the default theme, returns NULL on failure. Build the main window's UI into @ref r_sdl_frontend.root. Sizes are in logical pixels, high DPI is handled automatically.
//! @param flags Combination of @ref r_sdl_flags, 0 for a visible window of fixed size.
//! @see_also r_sdl_free r_sdl_update
//! @concepts frontend context
//@function r_sdl_init

//! A minimal main loop, rebuilding when the window size changes.
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

//! - `R_SDL_RESIZABLE`: the window can be resized, rebuild when @ref r_ui_list.resized of @ref r_sdl_frontend.root is set.
//! - `R_SDL_VSYNC`: synchronize presentation with the display refresh.
//! - `R_SDL_HIDDEN`: don't show the window, e.g. for screenshots.
//@enum r_sdl_flags

//! - `root`: UI list of the main window.
//! - `context`: the rcut context, for low level functions.
//! - `draw`: the rcut_draw instance, for themes and fonts.
//! - `no_popup_windows`: show popups inside the main window instead of separate windows.
//!
//! The remaining fields are internal.
//@struct r_sdl_frontend

//! Forwards an SDL event to rcut: mouse, keyboard, text input, resizes and close requests. Returns true if rcut used it. Pass all events, unrelated ones are ignored.
//!
//! Right Alt is AltGr on many layouts, it doesn't enter accelerator mode.
//! @concepts frontend input
//@function r_sdl_process_event

//! Returns true if the user requested to close the dialog window, `clear` resets the request, popup windows are closed automatically instead.
//! @see_also r_sdl_dialog r_destroy_ui_list
//! @concepts frontend target
//@function r_sdl_close_requested

//! Advances timers (hover delay, key repeat, caret blinking), removes expired ephemerals, updates the mouse cursor and starts or stops SDL text input. Call it once per loop iteration after processing events.
//! @concepts frontend rebuild
//@function r_sdl_update

//! Draws all windows. Only damaged areas are redrawn, calling it every iteration is cheap if nothing changed.
//! @concepts frontend draw-context
//@function r_sdl_render_ui

//! Renders the main window and saves it to `path` as PNG (BMP before SDL 3.4), returns false on failure.
//! @see_also r_sdl_render_ui
//! @concepts frontend draw-context
//@function r_sdl_screenshot

//! Frees the frontend and closes all its windows.
//! @see_also r_sdl_init
//! @concepts frontend
//@function r_sdl_free

//! Returns true if the user requested to quit, `clear` resets the request.
//! @see_also r_sdl_process_event r_sdl_close_requested
//! @concepts frontend
//@function r_sdl_quit_requested

//! Opens a new window and returns its root list, `root_id` being the ID of its root area. The list is modal. Build into it like into @ref r_sdl_frontend.root and close it with @ref r_destroy_ui_list, e.g. once @ref r_sdl_close_requested returns true. If no window is available it is shown centered in the main window instead.
//! @concepts frontend target ui_list
//@function r_sdl_dialog

//! Maximum number of windows, including popup windows.
//! @see_also r_sdl_dialog
//@define RCUT_SDL_MAX_WINDOWS

//! Size in pixels of the software cache shared by all windows, it must hold everything a frame uses.
//! @see_also r_draw_software_init
//@define RCUT_SDL_CACHE_PIXELS
