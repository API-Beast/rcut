#include <stdio.h>
#include "rcut_sdl.h"

static void on_ok(void) {}
static void on_select(int32_t index) { (void)index; }
static void on_close(void) {}

//@module core

//@section UI Lists
//! <TODO: Explain UI list concept>

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
	r_label(l, r_shrink(l->root_area, 16), "Hello from a second list");
}

//! <TODO: Describe destroy semantics>
//! @param list <TODO: List to destroy>
//! @see_also r_create_ui_list r_list_set_visibility
//@function r_destroy_ui_list

//@example
static void doc_r_destroy_ui_list(r_sdl_frontend* f, r_ui_list* ui)
{
	r_ui_list* dialog = r_sdl_dialog(f, 200, 320, 160, "Settings");
	if(r_sdl_close_requested(f, dialog, true))
		r_destroy_ui_list(dialog);
}

//! <TODO: Rewrite, explain ephemeral lifetime>
//! 
//! Ephemerals are UI list attached to a rectangle with automatic lifetime managment.
//!
//! Note here: Salt the r parameter here with @ref r_salt to allow for multiple popups on a single item.
//! @see_also r_popup r_viewport
//@function r_ephemeral_keep_alive

//! <TODO: Explain rebuild cycle>
//! 
//! Swap last_frame and current frame lists
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

//! <TODO: Explain when and why to call>
//! 
//! Reconcile interaction state with the previous frame.
//@function r_list_reconcile

//! Shows or hides the list, a hidden list and its child lists are not drawn and receive no input.
//! @see_also r_list_is_visible r_create_ui_list
//@function r_list_set_visibility

//! <TODO: Describe important fields>
//! @see_also r_create_ui_list r_list_clear
//@struct r_ui_list

//! <TODO: Rewrite as user docs>
//! 
//! This is the maximum number of elements a @ref r_ui_list can grow to.
//! This much virtual memory will be reserved. It will only be commited if needed.
//! Reserved virtual memory is a limited resource on 32 bit systems, but basically free on 64 systems, hence the different limits.
//@define RCUT_LIST_MAX_ELEMENTS

//! <TODO: Explain deferred ephemeral deletion>
//@function r_ephemeral_mark_to_delete

//! <TODO: Describe popup creation, flags>
//@function r_popup

//! <TODO: Describe flag combinations>
//@enum r_popup_flags

//! <TODO: Explain viewport lists, masking>
//@function r_viewport

//! <TODO: Explain scroll region usage>
//@function r_scroll_begin

//! <TODO: Explain content size update>
//@function r_scroll_end

//! <TODO: Describe scroll coordinates>
//@function r_scroll_to

//! Returns the bounding rectangle of all drawables in the list, in list space.
//! @see_also r_scroll_end rect_expand
//@function r_list_get_content_rect

//! <TODO: Describe origin and clip>
//@function r_list_get_transform

//! Returns false if the list or any of its parent lists is hidden.
//! @see_also r_list_set_visibility
//@function r_list_is_visible

//! Prints the list's drawables and interactables to stdout for debugging.
//! @see_also r_ui_list
//@function r_list_print

//! Maximum number of elements of a tooltip list, all other lists use @ref RCUT_LIST_MAX_ELEMENTS.
//! @see_also RCUT_LIST_MAX_ELEMENTS r_tooltip r_popup
//@define RCUT_EPHEMERAL_MAX_ELEMENTS

//@section Context
//! <TODO: Introduce context and frontends>

//! <TODO: Explain context ownership, usage>
//@struct r_context opaque

//! Initializes the context and reserves memory for its UI lists.
//! @see_also r_free_context r_context r_create_ui_list
//@function r_init_context

//! Frees all UI lists and memory owned by the context.
//! @see_also r_init_context
//@function r_free_context

//! <TODO: Describe frontend callbacks>
//@struct r_interface

//! <TODO: Explain per-frame time update>
//@function r_advance_time

//! <TODO: Explain ephemeral sweep, return>
//@function r_context_sweep

//! <TODO: Explain damage tracking>
//@function r_damage_id

//! <TODO: Explain when to call>
//@function r_damage_clear

//! Maximum number of regular UI lists per context.
//! @see_also RCUT_CONTEXT_MAX_EPHEMERAL_LISTS r_create_ui_list
//@define RCUT_CONTEXT_MAX_LISTS

//! Maximum number of ephemeral UI lists (popups, viewports, tooltips) per context.
//! @see_also RCUT_CONTEXT_MAX_LISTS r_popup r_viewport
//@define RCUT_CONTEXT_MAX_EPHEMERAL_LISTS

//@section Rects and IDs
//! <TODO: Explain rects and IDs>

//! <TODO: Explain rect ID field>
//@struct rect

//! <TODO: Explain salting and IDs>
//!
//! Some salts you can use freely (`R_SALT00` to `R_SALT20`). Don't worry any salt will do, you can also just write down a random number.
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

//! <TODO: Explain cutting rect sides>
//! @param dir <TODO: Side to cut from>
//! @param to_cut <TODO: Rect being cut down>
//! @param salt <TODO: Salt for new ID>
//! @see_also r_salt r_stack
//@function r_cut_px

//! <TODO: Caption for screenshot>
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

//! <TODO: Explain alignment flag combinations>
//@enum r_alignment

//! Returns the smallest rectangle containing both `a` and `b` with the ID of `a`, `rect_expand_pt` does the same for a point.
//! @see_also rect_intersect r_grow
//@function rect_expand

//! Returns the overlap of `a` and `b` with the ID of `a`, its size is 0 if they don't overlap.
//! @see_also rect_expand
//@function rect_intersect

//! Predefined salt values for @ref r_salt and the cutting functions. You can use these freely or use your own values. There are no "bad" salts other than reusing the same salt at multiple places.
//! @see_also r_salt r_cut_px
//@enum r_salts

//! <TODO: Describe fraction cutting>
//@function r_cut_fraction

//! Cuts one rectangle per entry of `r` from `to_cut`, sized by each entry's current width or height.
//! @see_also r_cut_px r_expand_group
//@function r_cut_group

//! Grows `to_expand` by `pixels` towards `dir` and returns the added area.
//! @see_also r_cut_px r_expand_group
//@function r_expand_px

//! Grows `to_expand` once per entry of `r`, sized by each entry's current width or height, and stores the added areas in `r`.
//! @see_also r_expand_px r_cut_group
//@function r_expand_group

//! Moves a rectangle by the given offset, keeping its ID.
//! @see_also r_grow r_aligned
//@function r_offset

//! Grows a rectangle by `pixels` on all sides, or per side with `r_grow_sides`, keeping its ID.
//! @see_also r_shrink r_offset
//@function r_grow

//! Shrinks a rectangle by `pixels` on all sides, or per side with `r_shrink_sides`, keeping its ID.
//! @see_also r_grow r_inner
//@function r_shrink

//! Enlarges the rectangle to at least `w` by `h` pixels.
//! @see_also r_fill
//@function r_min_size

//@section Layout
//! <TODO: Introduce layout iterators>

//! <TODO: Describe iterator fields>
//@struct r_layout_iter

//! How a layout iterator places its rectangles.
//@enum r_layout_type

//! <TODO: Explain stacking direction, spacing>
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

//! <TODO: Explain size argument, variants>
//@function r_next

//! <TODO: Caption for screenshot>
//@example screenshot
static void doc_r_next(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter cols = r_stack(ui, r_shrink(ui->root_area, 12), R_LEFT);
	for(int i = 0; i < 3; i++)
		r_button(ui, r_next_fraction(&cols, 1.0f / (3 - i)), (char*[]){ "Low", "Medium", "High" }[i]);
}

//! Returns all space left in the layout iterator.
//! @see_also r_next r_stack
//@function r_remaining

//! Returns the rectangle the layout iterator returned last, to place several elements on the same rectangle.
//@function r_last

//! <TODO: Rewrite, add usage example>
//! 
//! The rectangles should be initialized with their minimum sizes before calling.
//@function r_fill

//! <TODO: Explain style based spacing>
//@function r_layout

//! Skips `size` pixels of the layout iterator's remaining area.
//! @see_also r_next r_stack
//@function r_gap

//! <TODO: Explain nested row layouts>
//@function r_row

//! <TODO: Explain grouped layout backgrounds>
//@function r_group_begin

//! <TODO: Describe returned group rect>
//@function r_group_end

//! Starts a flow layout inside `parent`: elements are placed left to right and wrap into new lines.
//! The horizontal bits of `align` align each line, the vertical bits align the elements within a line.
//@function r_flow_begin

//! Adds word-wrapped text to a flow layout, split into one label per line, using the flow's style if `style` is 0.
//@function r_text

//@example screenshot
static void doc_r_text(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_layout_iter flow = r_flow_begin(&it, R_STYLE_LABEL, R_TOP_LEFT);
	r_text(&flow, 0, "Text wraps at word boundaries to fit the width of the flow layout.");
	r_flow_end(&flow, &it);
}

//! <TODO: Describe returned flow rect>
//@function r_flow_end

//! <TODO: Explain aligning inside target>
//@function r_align

//! <TODO: Explain returned aligned rect>
//@function r_aligned

//! <TODO: Explain placing beside target>
//@function r_aside

//! <TODO: Explain relative positioning>
//@function r_relative_to

//! Returns the content rectangle of an element with the style, given its outer rectangle.
//! @see_also r_outer r_draw_stylebox_inner_rect
//@function r_inner

//! Returns the outer rectangle an element with the style needs to fit the content rectangle `inner`.
//! @see_also r_inner r_measure r_draw_stylebox_outer_rect
//@function r_outer

//! Grows the outer rectangle by half the style's default spacing on every side.
//! @see_also r_remove_margins r_layout_default_spacing
//@function r_with_margins

//! Removes the margins added by @ref r_with_margins.
//! @see_also r_with_margins
//@function r_remove_margins

//! Returns the theme's default element size for the style, 28 if the frontend provides none.
//! @see_also r_layout_default_spacing r_theme_layout r_interface
//@function r_layout_default_size

//! Returns the theme's default spacing between elements of the style, 8 if the frontend provides none.
//! @see_also r_layout_default_size r_theme_layout
//@function r_layout_default_spacing

//@section Text measurement
//! <TODO: Introduce text measuring>

//! <TODO: Describe measuring variants>
//@function r_measure

//! <TODO: Explain inner/outer/margin rects>
//@function r_style_measure_all


//@section Widgets
//! <TODO: Introduce high level widgets>

//! <TODO: Describe button, add see_also>
//@function r_button

//@example screenshot
static void doc_r_button(r_sdl_frontend* f, r_ui_list* ui)
{
	rect ok = r_aligned(R_CENTER, ui->root_area, (rect){ .w = 100, .h = 32 });
	r_button(ui, ok, "OK");
	r_on_click(ui, ok, on_ok);
}

//! <TODO: Describe label variants>
//! @see_also r_text r_header
//@function r_label

//@example screenshot
static void doc_r_label(r_sdl_frontend* f, r_ui_list* ui)
{
	static float volume = 0.6f;
	// [...]
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_label(ui, r_next(&it, 24), "Plain label");
	r_label_value(ui, r_next(&it, 24), "Volume: %.2f", R_PTR(&volume));
}

//! <TODO: Explain click callback variants>
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

//! <TODO: Describe checkbox>
//! @param binding <TODO: Bound bool value>
//@function r_checkbox

//@example screenshot
static void doc_r_checkbox(r_sdl_frontend* f, r_ui_list* ui)
{
	static bool mute = true;
	static bool fullscreen = false;
	// [...]
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_checkbox(ui, r_next(&it, -1), "Mute", R_PTR(&mute));
	r_checkbox(ui, r_next(&it, -1), "Fullscreen", R_PTR(&fullscreen));
}

//! <TODO: Describe step and binding>
//! @see_also r_slider_box r_spinbox
//@function r_slider

//@example screenshot
static void doc_r_slider_h(r_sdl_frontend* f, r_ui_list* ui)
{
	static float volume = 0.6f;
	// [...]
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_label_value(ui, r_next(&it, 24), "Volume %.2f", R_PTR(&volume));
	r_slider_h(ui, r_next(&it, 24), 0, 1, 0.05f, R_PTR(&volume));
}

//@example screenshot
static void doc_r_slider_v(r_sdl_frontend* f, r_ui_list* ui)
{
	static float levels[3] = { 0.8f, 0.5f, 0.3f };
	// [...]
	r_layout_iter row = r_stack(ui, r_shrink(ui->root_area, 12), R_LEFT);
	for(int i = 0; i < 3; i++)
		r_slider_v(ui, r_next(&row, 24), 0, 1, 0.05f, R_PTR(&levels[i]));
}

//! <TODO: Explain style composition>
//@enum r_style

//! <TODO: Describe icon drawing>
//@function r_icon

//@example screenshot
static void doc_r_icon(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_LEFT);
	int icons[] = { R_ICON_HOME, R_ICON_FOLDER, R_ICON_SEARCH, R_ICON_SETTINGS, R_ICON_INFO };
	for(int i = 0; i < 5; i++)
		r_icon(ui, r_next(&it, 24), 0, icons[i]);
}

//! Draws a label with the header style.
//! @see_also r_label
//@function r_header

//@example screenshot
static void doc_r_header(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_header(ui, r_next(&it, 28), "Audio");
	r_label(ui, r_next(&it, 20), "Output device");
}

//! Draws a background panel, using `R_STYLE_PANEL` if `style` is 0.
//! @see_also r_style r_separator
//@function r_panel

//@example screenshot
static void doc_r_panel(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter rows = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	
	r_panel(ui, r_next(&rows, 64), 0);
	r_label(ui, r_shrink(r_last(&rows), 8), "R_STYLE_PANEL");

	r_panel(ui, r_next(&rows, 64), R_STYLE_PANEL | R_VARIANT_INSET);
	r_label(ui, r_shrink(r_last(&rows), 8), "R_VARIANT_INSET");

	r_panel(ui, r_next(&rows, 64), R_STYLE_PANEL | R_VARIANT_RAISED);
	r_label(ui, r_shrink(r_last(&rows), 8), "R_VARIANT_RAISED");
}

//! Draws a separator line through the center of `r`, horizontal if `r` is wider than tall, vertical otherwise.
//! @see_also r_panel r_split
//@function r_separator

//@example screenshot
static void doc_r_separator(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_label(ui, r_next(&it, 20), "Above");
	r_separator(ui, r_next(&it, 1));
	r_label(ui, r_next(&it, 20), "Below");
}

//! A labeled switch that works like @ref r_checkbox.
//! @see_also r_checkbox r_radio
//@function r_toggle

//@example screenshot
static void doc_r_toggle(r_sdl_frontend* f, r_ui_list* ui)
{
	static bool mute = true;
	static bool fullscreen = false;
	// [...]
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_toggle(ui, r_next(&it, -1), "Mute", R_PTR(&mute));
	r_toggle(ui, r_next(&it, -1), "Fullscreen", R_PTR(&fullscreen));
}

//! <TODO: Explain value matching binding>
//@function r_radio

//@example screenshot
static void doc_r_radio(r_sdl_frontend* f, r_ui_list* ui)
{
	static int quality = 1;
	// [...]
	r_layout_iter row = r_stack(ui, r_shrink(ui->root_area, 12), R_LEFT);
	char* labels[] = { "Low", "Medium", "High" };
	for(int i = 0; i < 3; i++)
		r_radio(ui, r_next_fraction(&row, 1.0f / (3 - i)), labels[i], i, R_PTR(&quality));
}

//! <TODO: Explain selectable list items>
//@function r_selectable

//@example screenshot
static void doc_r_selectable(r_sdl_frontend* f, r_ui_list* ui)
{
	static int selected = 1;
	// [...]
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 8), R_TOP);
	it.spacing = 2;
	char* files[] = { "notes.md", "budget.xlsx", "holiday.jpg" };
	for(int i = 0; i < 3; i++)
		r_selectable(ui, r_next(&it, 26), files[i], i, R_PTR(&selected));
}

//! <TODO: Explain drag and input>
//@function r_slider_box

//@example screenshot
static void doc_r_slider_box(r_sdl_frontend* f, r_ui_list* ui)
{
	static float brightness = 40;
	// [...]
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_slider_box(ui, r_next(&it, 26), "Brightness", 0, 100, 1, R_PTR(&brightness));
}

//! <TODO: Describe spinbox step buttons>
//@function r_spinbox

//@example screenshot
static void doc_r_spinbox(r_sdl_frontend* f, r_ui_list* ui)
{
	static float count = 3;
	// [...]
	rect r = r_aligned(R_CENTER, ui->root_area, (rect){ .w = 120, .h = 28 });
	r_spinbox(ui, r, 0, 99, 1, R_PTR(&count));
}

//! Draws a progress bar filled to the bound value within `min` to `max`, clamped to that range.
//@function r_progress

//@example screenshot
static void doc_r_progress(r_sdl_frontend* f, r_ui_list* ui)
{
	static float progress = 65.0f;
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_label(ui, r_next(&it, 20), "Downloading... 65%");
	r_progress(ui, r_next(&it, 8), 0, 100, R_PTR(&progress));
}

//! <TODO: Explain tab pages lists>
//@function r_tabs

//@example screenshot
static void doc_r_tabs(r_sdl_frontend* f, r_ui_list* ui)
{
	static int tab = 0;
	// [...]
	static char* labels[] = { "Files", "Recent" };
	r_ui_list* pages[2];
	r_tabs(ui, r_shrink(ui->root_area, 12), labels, 2, R_PTR(&tab), pages);
	r_label(pages[0], r_shrink(pages[0]->document_area, 8), "Contents of the first tab");
	r_label(pages[1], r_shrink(pages[1]->document_area, 8), "Contents of the second tab");
}

//! <TODO: Describe dropdown items, binding>
//@function r_dropdown

//@example screenshot
static void doc_r_dropdown(r_sdl_frontend* f, r_ui_list* ui)
{
	static int language = 0;
	// [...]
	static char* languages[] = { "English", "Deutsch", "Français" };
	rect r = r_aligned(R_TOP, r_shrink(ui->root_area, 16), (rect){ .w = 160, .h = 28 });
	r_dropdown(ui, r, languages, 3, R_PTR(&language));
	//@example_end
	r_post_mouse_down(&f->context, 0, R_MOUSE_LEFT, 110, 30);
	r_post_mouse_up  (&f->context, 0, R_MOUSE_LEFT, 110, 30);
	r_post_mouse_move(&f->context, 0, 110, 92);
}

//! <TODO: Describe segmented choices>
//@function r_segmented

//@example screenshot
static void doc_r_segmented(r_sdl_frontend* f, r_ui_list* ui)
{
	static int period = 1;
	// [...]
	static char* periods[] = { "Day", "Week", "Month" };
	rect r = r_aligned(R_CENTER, ui->root_area, (rect){ .w = 220, .h = 28 });
	r_segmented(ui, r, periods, 3, R_PTR(&period));
}

//! <TODO: Explain tree node parameters>
//@function r_tree_node

//@example screenshot
static void doc_r_tree_node(r_sdl_frontend* f, r_ui_list* ui)
{
	static int selected = 1;
	static bool tree_open[4] = { true };
	// [...]
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 8), R_TOP);
	it.spacing = 2;
	if(r_tree_node(ui, r_next_keyed(&it, 24, 0), 0, "Projects", R_ICON_FOLDER, true, R_PTR(&tree_open[0]), 0, R_PTR(&selected)))
	{
		r_tree_node(ui, r_next_keyed(&it, 24, 1), 1, "rcut_core.h", R_ICON_FILE, false, R_PTR(&tree_open[1]), 1, R_PTR(&selected));
		r_tree_node(ui, r_next_keyed(&it, 24, 2), 1, "rcut_draw.h", R_ICON_FILE, false, R_PTR(&tree_open[2]), 2, R_PTR(&selected));
	}
	r_tree_node(ui, r_next_keyed(&it, 24, 3), 0, "Photos", R_ICON_FOLDER, true, R_PTR(&tree_open[3]), 3, R_PTR(&selected));
}

//! <TODO: Explain split fraction, minimums>
//@function r_split

//@example screenshot
static void doc_r_split(r_sdl_frontend* f, r_ui_list* ui)
{
	static float split = 0.4f;
	// [...]
	rect left, right;
	r_split(ui, r_shrink(ui->root_area, 8), R_LEFT, R_PTR(&split), 60, 60, &left, &right);
	r_panel(ui, left,  0);
	r_panel(ui, right, 0);
	r_label(ui, r_shrink(left, 8),  "Left");
	r_label(ui, r_shrink(right, 8), "Right");
}

//! <TODO: Describe single line editing>
//@function r_line_edit

//@example screenshot
static void doc_r_line_edit(r_sdl_frontend* f, r_ui_list* ui)
{
	static char name[64] = "";
	static char search[64] = "rcut";
	// [...]
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_line_edit(ui, r_next(&it, -1), name, sizeof(name), "Your name");
	r_line_edit(ui, r_next(&it, -1), search, sizeof(search), "Search");
	//@example_end
	r_post_mouse_down(&f->context, 0, R_MOUSE_LEFT, 240, 68);
	r_post_mouse_up  (&f->context, 0, R_MOUSE_LEFT, 240, 68);
}

//! <TODO: Describe multiline editing>
//@function r_text_area

//@example screenshot
static void doc_r_text_area(r_sdl_frontend* f, r_ui_list* ui)
{
	static char notes[256] = "Meeting notes\n- Ship the docs\n- Review the theme";
	// [...]
	r_text_area(ui, r_shrink(ui->root_area, 12), notes, sizeof(notes), "Write something...");
	//@example_end
	r_post_mouse_down(&f->context, 0, R_MOUSE_LEFT, 240, 90);
	r_post_mouse_up  (&f->context, 0, R_MOUSE_LEFT, 240, 90);
}

//! <TODO: Describe style and multiline>
//@function r_text_edit_ex

//@example screenshot
static void doc_r_text_edit_ex(r_sdl_frontend* f, r_ui_list* ui)
{
	static char search[64] = "rcut";
	// [...]
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_text_edit_ex(ui, r_next(&it, -1), search, sizeof(search), "Search", R_STYLE_INPUT | R_VARIANT_FLAT, false);
	//@example_end
	r_post_mouse_down(&f->context, 0, R_MOUSE_LEFT, 200, 28);
	r_post_mouse_up  (&f->context, 0, R_MOUSE_LEFT, 200, 28);
}

//! <TODO: Explain editor state storage>
//@struct r_text_edit_state opaque

//! Shows `text` (at most 511 bytes) in a tooltip while the mouse hovers `r`.
//! @see_also r_popup RCUT_MOUSE_*
//@function r_tooltip

//@example
static void doc_r_tooltip(r_sdl_frontend* f, r_ui_list* ui)
{
	rect ok = r_aligned(R_CENTER, ui->root_area, (rect){ .w = 100, .h = 32 });
	r_button(ui, ok, "OK");
	r_tooltip(ui, ok, "Applies the settings and closes the window.");
}

//! <TODO: Explain dialog content area>
//@function r_dialog_begin

//@example
static void doc_r_dialog_begin(r_sdl_frontend* f, r_ui_list* ui)
{
	static char name[64] = "";
	// [...]
	r_ui_list* dialog = r_sdl_dialog(f, 300, 320, 140, "Settings");
	rect area = r_dialog_begin(dialog);
	r_layout_iter it = r_stack(dialog, area, R_TOP);
	r_label(dialog, r_next(&it, 20), "Display name");
	r_line_edit(dialog, r_next(&it, -1), name, sizeof(name), "Your name");
}

//! <TODO: Describe built-in icons>
//@enum r_icon_variant

//@section Menus
//! <TODO: Introduce menus and context menus>

//! <TODO: Describe item fields, submenus>
//@struct r_menu_item

//! <TODO: Explain menu array termination>
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
//@function r_menu_popup

//! <TODO: Explain context menu entries>
//@function r_on_context

//! <TODO: Describe popup placement rules>
//@function r_popup_place

//@section Data binding
//! <TODO: Explain binding concept>

//! <TODO: Explain pointer vs getter/setter>
//@struct r_data_binding

//! <TODO: Describe pointer binding>
//@define R_PTR

//@example
static void doc_R_PTR(r_sdl_frontend* f, r_ui_list* ui)
{
	static bool mute = true;
	// [...]
	r_checkbox(ui, ui->root_area, "Mute", R_PTR(&mute));
}

//! Binds `LENGTH` bits starting at bit `OFFSET` of the integer at `BASE`.
//! @see_also R_PTR R_GETSET r_data_binding
//@define R_BITFIELD

//! <TODO: Explain getter/setter binding>
//@define R_GETSET

//! <TODO: Explain getter/setter fields>
//@struct r_data_desc

//! Type of the value a pointer binding refers to.
//! @see_also R_PTR r_data_binding
//@enum r_data_type

//! Writes an integer to the binding, converted to the bound type.
//! @see_also r_geti r_setd r_data_binding
//@function r_seti

//! Reads the binding as an integer.
//! @see_also r_seti r_getd
//@function r_geti

//! Writes a double to the binding, converted to the bound type.
//! @see_also r_getd r_seti
//@function r_setd

//! Reads the binding as a double.
//! @see_also r_setd r_geti
//@function r_getd

//@section Low level utility functions
//! <TODO: Introduce low level utilities>

//! Returns true if the element should look pressed: it is pressed and the mouse is still over it.
//! @see_also r_draw_take_damage
//@function r_is_pressed_visual

//! <TODO: Explain event dispatch, return>
//@function r_dispatch

//! Moves the keyboard focus to `new_focus`, or clears it if NULL, and sends `R_FOCUS_EXIT` and `R_FOCUS_ENTER`.
//! @see_also r_event_type r_focus_wants_text
//@function r_change_focus

//! Returns true if the focused element accepts text input.
//! @see_also r_post_text r_change_focus
//@function r_focus_wants_text

//! Returns the mouse cursor of the hovered or dragged element.
//! @see_also r_cursor
//@function r_hover_cursor

//! Mouse cursor shapes, see @ref r_hover_cursor.
//! @see_also r_hover_cursor r_interact
//@enum r_cursor

//! <TODO: Explain only_topmost, return>
//@function r_close_popups

//@section Low level input
//! <TODO: Introduce frontend input posting>

//! <TODO: Describe return value, modifiers>
//! @param keycode Keys that are special cased by rcut use Windows virtual key codes (`R_KEY_*`), frontends need to map their keys to these. Letters are expected as uppercase ASCII ('A'-'Z'), digits as ASCII.
//@function r_post_key_input

//! <TODO: Describe coordinates, return value>
//! @param mouse_button `R_MOUSE_LEFT`, `R_MOUSE_MIDDLE` or `R_MOUSE_RIGHT` (same numbering as SDL).
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
//@function r_post_mouse_move

//! Sends a mouse button release in target space to the context and returns the ID of the element that handled it, or 0.
//! @see_also r_post_mouse_down r_post_mouse_move
//@function r_post_mouse_up

//! Sends a scroll event to the elements under the mouse, topmost first, and returns the ID of the one that handled it, or 0.
//! @see_also r_post_mouse_move r_scroll_begin
//@function r_post_mouse_wheel

//! Sends committed text to the focused element, `r_post_text_edit` sends IME composition text, both return the ID of the element that handled it, or 0.
//! @see_also r_post_key_input r_focus_wants_text
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

//@section Low level UI building
//! <TODO: Introduce low level building>

//! <TODO: Explain storage lifetime>
//! 
//! Returns a pointer to a block of memory with lifetime scoped to this rebuild.
//@function r_storage_alloc

//! <TODO: Explain storage lifetime>
//! 
//! `r_storage_alloc` + copy, zeroed if data is NULL.
//@function r_store

//@section Drawables
//! <TODO: Introduce drawables>

//! <TODO: Describe drawable fields>
//@struct r_drawable

//! <TODO: Explain raw style drawables>
//@function r_style

//! <TODO: Explain labeled style drawables>
//@function r_labeled

//! Returns the drawable with the given ID from the current frame, or NULL.
//! @see_also r_list_find_last_frame_drawable r_drawable
//@function r_list_find_this_frame_drawable

//! <TODO: Explain last frame lookup>
//@function r_list_find_last_frame_drawable

//! <TODO: Describe dynamic rect kinds>
//@enum r_dynamic_rect_kind

//! <TODO: Describe dynamic rect data>
//@struct r_dynamic_rect_data

//! <TODO: Explain dynamic rect evaluation>
//@function r_dynamic_rect

//! <TODO: Explain drawable evaluation>
//@function r_drawable_eval

//! <TODO: Describe bound thumb drawables>
//@function r_drawable_thumb

//! <TODO: Describe bound fill drawables>
//@function r_drawable_fill

//! <TODO: Describe bound toggle drawables>
//@function r_drawable_toggled

//! <TODO: Describe custom update drawables>
//@function r_drawable_custom

//! <TODO: Explain change detection, damage>
//@function r_drawable_take_change

//@section Interactables
//! <TODO: Introduce interactables>

//! <TODO: Describe interactable fields>
//@struct r_interact

//! <TODO: Explain interactable creation, reuse>
//@function r_interactable

//! <TODO: Explain mouse capture groups>
//@function r_set_press_group

//! Returns the visible interactable with the given ID, or NULL.
//! @see_also r_list_find_this_frame_interact r_interact
//@function r_find_interact

//! Returns the interactable with the given ID from the current frame, or NULL.
//! @see_also r_list_find_last_frame_interact r_find_interact
//@function r_list_find_this_frame_interact

//! <TODO: Explain last frame lookup>
//@function r_list_find_last_frame_interact

//! <TODO: Describe callback fields>
//@struct r_callback

//! <TODO: Explain callback attachment>
//@function r_attach_callback

//! <TODO: Explain callback data storage>
//@function r_callback_set_data

//! <TODO: Rewrite, explain data sharing>
//! 
//! Don't share data from last frames callback. Use @ref r_copy_last_frame_data instead.
//@function r_callback_share_data

//! <TODO: Explain persistent callback data>
//@function r_copy_last_frame_data

//@module draw

//@section Renderer
//! <TODO: Introduce renderer and backends>

//! <TODO: Explain renderer ownership, usage>
//@struct r_draw opaque

//! <TODO: Explain backend and scale>
//@function r_draw_init

//! Frees all fonts, caches and memory owned by the renderer.
//! @see_also r_draw_init
//@function r_draw_free

//! <TODO: Explain rendering a target>
//@function r_draw_render

//! <TODO: Explain partial rendering>
//@function r_draw_render_area

//! <TODO: Explain damage rect usage>
//@function r_draw_take_damage

//! <TODO: Explain when bounds are needed>
//@function r_draw_target_bounds

//! Sets the DPI scale factor.
//! This clears the cached stylebox and fonts, so they are recomputed the next time they are rendered.
//! @see_also r_sdl_set_scale r_draw_clear_cache
//@function r_draw_set_scale

//! <TODO: Explain when to clear>
//@function r_draw_clear_cache

//! <TODO: Describe backend callbacks>
//@struct r_draw_backend

//! <TODO: Describe quad fields>
//@struct r_draw_quad

//! <TODO: Describe matrix and clip>
//@struct r_draw_transform

//! Rectangle with float coordinates. These are only used by @ref r_draw_quad and passed to the renderer interface that way.
//! @see_also r_draw_quad
//@struct r_frect

//@section Software renderer
//! <TODO: Introduce software backend>

//! <TODO: Explain software backend usage>
//@struct r_draw_software opaque

//! <TODO: Explain cache size>
//@function r_draw_software_init

//! Frees the buffers owned by the software renderer.
//! @see_also r_draw_software_init
//@function r_draw_software_free

//! <TODO: Explain backend creation>
//@function r_draw_software_backend

//! <TODO: Describe target pixel buffer>
//@function r_draw_software_set_target

//@section Colors
//! <TODO: Introduce color format>

//! Packs 8 bit red, green and blue components into an opaque ARGB color.
//! @see_also r_rgba_u8 r_rgb_f r_hex
//@function r_rgb_u8

//! Packs 8 bit red, green, blue and alpha components into an ARGB color.
//! @see_also r_rgb_u8 r_rgba_f r_hexa
//@function r_rgba_u8

//! Packs red, green and blue components in the range 0 to 1 into an opaque ARGB color.
//! @see_also r_rgba_f r_rgb_u8
//@function r_rgb_f

//! Packs red, green, blue and alpha components in the range 0 to 1 into an ARGB color.
//! @see_also r_rgb_f r_rgba_u8
//@function r_rgba_f

//! Turns a 0xRRGGBB value into an opaque ARGB color.
//! @see_also r_hexa r_rgb_u8
//@function r_hex

//! Turns a 0xRRGGBB value and an alpha in the range 0 to 1 into an ARGB color.
//! @see_also r_hex r_rgba_u8
//@function r_hexa

//@section Theme
//! <TODO: Introduce theme building API>

//! Sets the label color, `r_theme_label_color` uses it exactly as given while `r_theme_label_color_auto` adapts it to stay readable on the element's background.
//! @see_also r_theme_label r_theme_surface
//@function r_theme_label_color

//! <TODO: Describe theme options>
//@struct r_draw_theme_options

//! <TODO: Explain default theme setup>
//@function r_theme_default

//! <TODO: Describe palette colors>
//@struct r_draw_palette

//! <TODO: Explain theme rule builder>
//@function r_theme

//! <TODO: Explain tweaking existing rules>
//@function r_theme_tweak

//! <TODO: Explain build variants, states>
//@function r_theme_build

//! Fills the background with a vertical gradient from `top` to `bottom`.
//! @see_also r_theme_surface r_theme_tint
//@function r_theme_fill

//! Sets the color assumed behind a translucent background, used to pick readable label colors.
//! @see_also r_theme_fill r_theme_label_color
//@function r_theme_surface

//! Blends `color` over the background and border colors set so far.
//! @see_also r_theme_fill r_theme_border
//@function r_theme_tint

//! Sets the border width and color, `r_theme_border_color` only changes the color.
//! @see_also r_theme_underline r_theme_bevel r_theme_border_split
//@function r_theme_border

//! Replaces the border with a bottom border of the given width and color.
//! @see_also r_theme_border
//@function r_theme_underline

//! Sets separate top and bottom border colors.
//! @see_also r_theme_border r_theme_border_split
//@function r_theme_bevel

//! Sets the border width with separate top-left and bottom-right colors, for sunken or raised edges.
//@function r_theme_border_split

//! Sets the radius of all corners.
//! @see_also r_theme_corners
//@function r_theme_radius

//! Sets the radius of each corner individually.
//! @see_also r_theme_radius
//@function r_theme_corners

//! Sets the horizontal and vertical content padding.
//! @see_also r_theme_layout r_inner
//@function r_theme_padding

//! Sets a drop shadow with vertical offset, blur radius and opacity.
//! @see_also r_theme_shadow2 r_theme_no_shadow r_theme_inset
//@function r_theme_shadow

//! Sets a second drop shadow, drawn in addition to the one from @ref r_theme_shadow.
//! @see_also r_theme_shadow r_theme_glow
//@function r_theme_shadow2

//! Removes both drop shadows.
//! @see_also r_theme_shadow r_theme_shadow2
//@function r_theme_no_shadow

//! Sets an inner shadow with vertical offset, blur radius and opacity.
//! @see_also r_theme_shadow
//@function r_theme_inset

//! Adds a glow of the given color and width, using the second shadow if the first one is set.
//! @see_also r_theme_shadow r_theme_shadow2
//@function r_theme_glow

//! Sets the label color, adapted to stay readable, and the label alignment.
//! @see_also r_theme_label_color r_theme_font
//@function r_theme_label

//! Sets the label font.
//! @see_also r_theme_label r_draw_get_font
//@function r_theme_font

//! Sets the icon color and size.
//! @see_also r_icon
//@function r_theme_icon

//! Sets the default size and spacing returned by @ref r_layout_default_size and @ref r_layout_default_spacing.
//! @see_also r_layout_default_size r_layout_default_spacing r_theme_padding
//@function r_theme_layout

//! Enables or disables the focus ring.
//@function r_theme_focus

//! Focus indicator styles for the `r_theme_focus` function.
//@enum r_theme_focus

//! Draws labels with an outline of the given width and color.
//! @see_also r_theme_text_shadow
//@function r_theme_text_outline

//! Draws labels with a shadow of the given offset, blur and color.
//! @see_also r_theme_text_outline r_theme_shadow
//@function r_theme_text_shadow

//! <TODO: Describe style rule fields>
//@struct r_draw_style_rule

//! Returns the style rule for exactly `key`, or NULL.
//! @see_also r_draw_set_style_rule r_draw_find_style_rule r_draw_style_rule
//@function r_draw_get_style_rule

//! <TODO: Explain style fallback lookup>
//@function r_draw_find_style_rule

//! Copies `rule` as the style rule for `key`, replacing any existing one.
//! @see_also r_draw_get_style_rule r_draw_delete_style_rule
//@function r_draw_set_style_rule

//! Removes the style rule for `key`.
//! @see_also r_draw_set_style_rule
//@function r_draw_delete_style_rule

//@section Styleboxes
//! <TODO: Introduce stylebox concept>

//! <TODO: Describe slicing parameters>
//! 
//! `r_draw_upload_stylebox_fill`: Upload a aspect ratio aware fill to be used with the stylebox. It will be drawn under the rest of the stylebox. (The center of the stylebox must be transparent.)
//@function r_draw_upload_stylebox

//! Given the outer rect, calculates the inner rect (e.g. where text should be positioned).
//! @see_also r_draw_stylebox_outer_rect r_inner
//@function r_draw_stylebox_inner_rect

//! Given the inner rect, calculates the outer rect (e.g. how big the entire drawable needs to be to contain the contents of inner).
//! @see_also r_draw_stylebox_inner_rect r_outer
//@function r_draw_stylebox_outer_rect

//! <TODO: Describe stylebox definition>
//@struct r_draw_stylebox_def

//! <TODO: Describe gradient fields>
//@struct r_draw_gradient

//! <TODO: Explain generated styleboxes>
//@function r_draw_generate_stylebox

//! Frees the stylebox slot and its cached textures.
//! @see_also r_draw_generate_stylebox r_draw_upload_stylebox
//@function r_draw_free_stylebox

//! Sets a solid border of the given color, thickness and corner radius on all sides.
//! @see_also r_draw_stylebox_def r_draw_generate_stylebox
//@function r_stylebox_set_outline_border

//! <TODO: Describe shadow helper>
//@function r_stylebox_set_shadow

//! Sets a solid background color.
//! @see_also r_stylebox_set_bg_vertical_gradient r_draw_stylebox_def
//@function r_stylebox_set_bg_color

//! <TODO: Describe gradient helper>
//@function r_stylebox_set_bg_vertical_gradient

//! <TODO: Describe raised border helper>
//@function r_stylebox_set_raised_borders_simple

//! Number of stylebox slots.
//! @see_also RCUT_DRAW_USER_STYLEBOXES r_draw_generate_stylebox
//@define RCUT_DRAW_MAX_STYLEBOXES

//! First stylebox slot for the application, themes only use the slots below.
//! @see_also RCUT_DRAW_MAX_STYLEBOXES r_draw_upload_stylebox
//@define RCUT_DRAW_USER_STYLEBOXES

//@section Icons
//! <TODO: Introduce icon definitions>

//! <TODO: Describe icon ops format>
//! 
//! Icons use the same underlying caching system as fonts, caching each "font size" of the icon as needed.
//@function r_draw_upload_icon

//! <TODO: Describe icon op list>
//@struct r_draw_icon_def

//! <TODO: Add icon op example>
//@enum r_draw_icon_op

//! Returns the largest square centered in `outer`.
//! @see_also r_draw_upload_icon r_draw_render_icon
//@function r_draw_icon_get_rect

//@section Fonts
//! <TODO: Introduce font handling>

//! Selects the text backend: `RCUT_USE_HARFBUZZ`, `RCUT_USE_HB_MINI` or `RCUT_USE_STB_TRUETYPE`, if none is defined the backend is detected from previously included headers.
//! @see_also r_draw_font_register
//@define RCUT_USE_*

//! <TODO: Describe return rect>
//! 
//! Widths are measured to the nearest pixel, so text may overhang its measured width by up to half a pixel (+1 device pixel of truncation).
//@function r_draw_measure_text

//! <TODO: Explain font handle usage>
//@struct r_draw_font opaque

//! <TODO: Describe weight and style flags>
//@enum r_draw_font_flags

//! <TODO: Explain system font discovery>
//@function r_draw_find_system_fonts

//! <TODO: Explain slots and lazy loading>
//@function r_draw_font_register

//! <TODO: Explain fallback order>
//@function r_draw_font_set_fallbacks

//! <TODO: Explain font instance lookup>
//@function r_draw_get_font

//! Returns the line height of the font in unscaled pixels.
//! @see_also r_draw_get_font r_draw_measure_text
//@function r_draw_font_line_height

//! <TODO: Explain line breaking, force>
//@function r_draw_break_text

//@section Rendering
//! <TODO: Introduce manual render passes>

//! <TODO: Explain render pass usage>
//@function r_draw_render_pass_begin

//! <TODO: Explain render pass end>
//@function r_draw_render_pass_end

//! <TODO: Describe drawable rendering>
//@function r_draw_render_drawable

//! Draws an icon centered in `bounds` in the current render pass.
//! @see_also r_draw_render_pass_begin r_draw_upload_icon
//@function r_draw_render_icon

//! Draws text at the top left of `bounds` in the current render pass.
//! @see_also r_draw_render_pass_begin r_draw_measure_text
//@function r_draw_render_text

//! Draws the stylebox in slot `stylebox` stretched to `outer` in the current render pass.
//! @see_also r_draw_render_pass_begin r_draw_generate_stylebox
//@function r_draw_render_stylebox

//! Draws a solid rectangle in the current render pass.
//! @see_also r_draw_render_pass_begin
//@function r_draw_render_fill

//@module sdl

//! <TODO: Describe frontend creation>
//! @param flags <TODO: r_sdl_flags combination>
//! @see_also r_sdl_free r_sdl_update
//@function r_sdl_init

//! <TODO: Caption for main loop>
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

//! <TODO: Describe each flag>
//@enum r_sdl_flags

//! <TODO: Describe important fields>
//@struct r_sdl_frontend

//! <TODO: Describe event forwarding>
//!
//! Right Alt is AltGr on many layouts, it doesn't enter accelerator mode.
//@function r_sdl_process_event

//! Returns true if the user requested to close the dialog window, `clear` resets the request, popup windows are closed automatically instead.
//! @see_also r_sdl_dialog r_destroy_ui_list
//@function r_sdl_close_requested

//! <TODO: Explain update responsibilities>
//@function r_sdl_update

//! <TODO: Explain rendering step>
//@function r_sdl_render_ui

//! Renders the main window and saves it to `path` as PNG (BMP before SDL 3.4), returns false on failure.
//! @see_also r_sdl_render_ui
//@function r_sdl_screenshot

//! Frees the frontend and closes all its windows.
//! @see_also r_sdl_init
//@function r_sdl_free

//! Returns true if the user requested to quit, `clear` resets the request.
//! @see_also r_sdl_process_event r_sdl_close_requested
//@function r_sdl_quit_requested

//! <TODO: Explain dialog windows>
//@function r_sdl_dialog

//! Sets the UI scale factor, or follows the display scale if `scale` is 0 or less.
//! @see_also r_draw_set_scale
//@function r_sdl_set_scale

//! Maximum number of windows, including popup windows.
//! @see_also r_sdl_dialog
//@define RCUT_SDL_MAX_WINDOWS

//! Size in pixels of the software cache shared by all windows, it must hold everything a frame uses.
//! @see_also r_draw_software_init
//@define RCUT_SDL_CACHE_PIXELS

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
	DOC(doc_r_panel, 260, 90),
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
