//@module core_extended

//! <NOTE:Mostly unfinished.>

//@section Image buffers

//! Image buffers exist to pass custom image data directly to rcut.
//! @ref r_drawable has a `image_data` parameter, which, if set, applies the image as background of the drawable.
//! There is no constructor or destructor for r_image_data, it is expected of you to declare them as you would static, program lifetime data.
//! The built-in software renderer from rcut_draw will simply reference your texture data and not duplicate it. In that case you need to keep the texture in memory even if already cached.
//! <TODO:Rewrite.>
//! @see_also r_image r_image_invalidate

//@struct r_image_buffer

//! `R_IMAGE_UPLOAD_ONCE` is uploaded on first draw and then set to `R_IMAGE_UPLOAD_FINISHED`, for static image data or image data that changes rarely. `R_IMAGE_UPLOAD_ALWAYS` is uploaded and redrawn every frame, for dynamic content.
//@enum r_image_upload

//! How the image is fitted into the target rect.
//@enum r_image_mode

//! Draws `img` in `r` according to @ref r_image_buffer.mode.
//! @see_also r_image_buffer r_image_invalidate
//! @concepts image drawable
//@function r_image

//! A 16x16 checker drawn in each mode.
//@example screenshot
static void doc_r_image(r_sdl_frontend* f, r_ui_list* ui)
{
	static uint32_t px[16 * 16];
	static r_image_buffer img[4];
	for(int i = 0; i < 16 * 16; i++)
		px[i] = ((i % 16) / 4 + (i / 16) / 4) % 2 ? 0xFF3070C0 : 0xFFE0E0E0;
	rect row = r_shrink(ui->root_area, 8);
	for(int m = 0; m < 4; m++)
	{
		img[m] = (r_image_buffer){ .pixels = px, .w = 16, .h = 16, .stride = 16, .upload = R_IMAGE_UPLOAD_ONCE, .mode = m };
		r_image(ui, r_cut_px(R_LEFT, &row, 64, R_SALT00 + m), &img[m]);
		r_cut_px(R_LEFT, &row, 8, R_SALT04 + m);
	}
}


//@section Dynamic rects
//! <TODO: Explain dynamic rects>

//! <TODO: Explain dynamic rect evaluation>
//! @concepts dynamic_rect binding
//@function r_dynamic_rect

//! <TODO: Explain drawable evaluation>
//! @concepts dynamic_rect drawable
//@function r_drawable_eval

//! <TODO: Describe bound thumb drawables>
//! @concepts dynamic_rect drawable binding
//@function r_drawable_thumb

//! <TODO: Describe bound fill drawables>
//! @concepts dynamic_rect drawable binding
//@function r_drawable_fill

//! <TODO: Describe bound toggle drawables>
//! @concepts dynamic_rect drawable binding
//@function r_drawable_toggled

//! <TODO: Describe custom update drawables>
//! @concepts dynamic_rect drawable
//@function r_drawable_custom

//@section Callbacks
//! <TODO: Explain callbacks>

//! <TODO: Explain callback attachment>
//! @concepts callback interactable
//@function r_attach_callback

//! <TODO: Explain callback data storage>
//! @concepts callback storage
//@function r_callback_set_data

//! <TODO: Rewrite, explain data sharing>
//! 
//! Don't share data from last frames callback. Use @ref r_copy_last_frame_data instead.
//! @concepts callback storage
//@function r_callback_share_data

//! <TODO: Explain persistent callback data>
//! @concepts callback rebuild storage
//@function r_copy_last_frame_data

//@section Menus
//! <TODO: Explain menus>

//! <TODO: Describe item fields, submenus>
//@struct r_menu_item

//! <TODO: Explain menu array termination>
//! @concepts menu callback ephemeral
//@function r_menubar

//@example screenshot
static void doc_r_menubar(r_sdl_frontend* f, r_ui_list* ui)
{
	static const r_menu_item file_menu[] = {
		{ .label = "New",  .icon = R_ICON_FILE, .shortcut = "Ctrl+N", .on_click = on_ok },
		{ .label = "Quit", .icon = R_ICON_CLOSE, .on_click = on_close },
		{ 0 },
	};
	static const r_menu_item menus[] = {
		{ .label = "File", .submenu = file_menu },
		{ .label = "Edit", .submenu = file_menu },
		{ .label = "Help", .submenu = file_menu },
		{ 0 },
	};
	rect area = ui->root_area;
	r_menubar(ui, r_cut_px(R_TOP, &area, 30, R_SALT00), menus);
	//@example_end
	r_post_mouse_move(&f->context, 0, 20, 15);
	r_post_mouse_down(&f->context, 0, R_MOUSE_LEFT, 20, 15);
	r_post_mouse_up  (&f->context, 0, R_MOUSE_LEFT, 20, 15);
	r_post_mouse_move(&f->context, 0, 60, 46);
}

//! <TODO: Describe popup menu placement>
//! @concepts menu callback ephemeral
//@function r_menu_popup

//! <TODO: Explain context menu entries>
//! @concepts menu callback ephemeral
//@function r_on_context

//@section Storage
//! <TODO: Explain storage>

//! <TODO: Explain storage lifetime>
//! 
//! Returns a pointer to a block of memory with lifetime scoped to this rebuild.
//! @concepts storage rebuild
//@function r_storage_alloc

//! <TODO: Explain storage lifetime>
//! 
//! `r_storage_alloc` + copy, zeroed if data is NULL.
//! @concepts storage rebuild
//@function r_store

//@section Context
//! <TODO: Explain context>

//! <TODO: Explain context ownership, usage>
//@struct r_context opaque

//! Initializes the context and reserves memory for its UI lists.
//! @see_also r_free_context r_context r_create_ui_list
//! @concepts context
//@function r_init_context

//! Frees all UI lists and memory owned by the context.
//! @see_also r_init_context
//! @concepts context
//@function r_free_context

//! <TODO: Explain per-frame time update>
//! @concepts context input
//@function r_advance_time

//! <TODO: Explain ephemeral sweep, return>
//! @concepts context ephemeral rebuild
//@function r_context_sweep

//@section Frontends
//! <TODO: Explain frontends>

//! <TODO: Describe frontend callbacks>
//@struct r_interface

//@section Damage
//! <TODO: Explain damage>

//! <TODO: Explain damage tracking>
//! @concepts damage
//@function r_damage_id

//! <TODO: Explain when to call>
//! @concepts damage
//@function r_damage_clear

//! Maximum number of regular UI lists per context.
//! @see_also RCUT_CONTEXT_MAX_EPHEMERAL_LISTS r_create_ui_list
//@define RCUT_CONTEXT_MAX_LISTS

//! Maximum number of ephemeral UI lists (popups, viewports, tooltips) per context.
//! @see_also RCUT_CONTEXT_MAX_LISTS r_popup r_viewport
//@define RCUT_CONTEXT_MAX_EPHEMERAL_LISTS

//@section Input
//! <TODO: Explain input>

//! <TODO: Explain event dispatch, return>
//! @concepts input interactable
//@function r_dispatch

//! <TODO: Describe return value, modifiers>
//! @param keycode Keys that are special cased by rcut use Windows virtual key codes (`R_KEY_*`), frontends need to map their keys to these. Letters are expected as uppercase ASCII ('A'-'Z'), digits as ASCII.
//! @concepts input frontend
//@function r_post_key_input

//! <TODO: Describe coordinates, return value>
//! @param mouse_button `R_MOUSE_LEFT`, `R_MOUSE_MIDDLE` or `R_MOUSE_RIGHT` (same numbering as SDL).
//! @concepts input frontend
//@function r_post_mouse_down

//! Mouse timings in seconds and the drag threshold in pixels:
//! 
//! - `RCUT_MOUSE_DRAG_DISTANCE_DEFAULT`: Pixels the mouse must move while pressed before a drag starts.
//! - `RCUT_MOUSE_PRESSED_INITIAL_REPEAT`: Seconds a mouse button must be held before `R_MOUSE_PRESSED` starts repeating.
//! - `RCUT_MOUSE_PRESSED_REPEAT`: Seconds between repeated `R_MOUSE_PRESSED` events.
//! - `RCUT_MOUSE_HOVER_INITIAL_DELAY`: Seconds the mouse must rest on an element before the first `R_MOUSE_HOVER` event.
//! - `RCUT_MOUSE_HOVER_REPEAT`: Seconds between repeated `R_MOUSE_HOVER` events.
//! - `RCUT_MOUSE_MULTI_CLICK_TIME`: Maximum seconds between clicks on the same element to count as a double or triple click.
//! @see_also r_event_type r_advance_time
//@define RCUT_MOUSE_*

//! Sends a mouse move in target space to the context and returns the ID of the element that handled it, or 0.
//! @see_also r_post_mouse_down r_post_mouse_up r_post_mouse_wheel
//! @concepts input frontend
//@function r_post_mouse_move

//! Sends a mouse button release in target space to the context and returns the ID of the element that handled it, or 0.
//! @see_also r_post_mouse_down r_post_mouse_move
//! @concepts input frontend
//@function r_post_mouse_up

//! Sends a scroll event to the elements under the mouse, topmost first, and returns the ID of the one that handled it, or 0.
//! @see_also r_post_mouse_move r_scroll_begin
//! @concepts input frontend
//@function r_post_mouse_wheel

//! Sends committed text to the focused element, `r_post_text_edit` sends IME composition text, both return the ID of the element that handled it, or 0.
//! @see_also r_post_key_input r_focus_wants_text
//! @concepts input frontend
//@function r_post_text

//! Modifier key flags for @ref r_post_key_input.
//! @see_also r_post_key_input
//@enum r_modifier_flags

//! <TODO: Describe event order>
//@enum r_event_type

//! <TODO: Describe event fields>
//@struct r_event

//! Mouse buttons, numbered the same as in SDL.
//! @see_also r_post_mouse_down r_post_mouse_up
//@define R_MOUSE_*

//! Key codes of the keys rcut handles specially, they are Windows virtual key codes that frontends need to map their keys to; letters are expected as uppercase ASCII ('A'-'Z') and digits as ASCII.
//! @see_also r_post_key_input
//@define R_KEY_*
