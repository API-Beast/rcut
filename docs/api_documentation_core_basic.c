//@module core_basic

//@section Rects

//@struct rect
//! Everything in rcut is based on rectangles.

//! Cuts `pixels` from the `dir` side of `to_cut` and returns the cut off part. `to_cut` shrinks accordingly, so successive calls keep carving up the remaining area. If `pixels` exceeds the available size the result takes everything and `to_cut` is left empty.
//!
//! The returned rect gets a new ID derived from `to_cut`'s ID and `salt`.
//! @param dir Side to cut from: `R_TOP`, `R_BOTTOM`, `R_LEFT` or `R_RIGHT`.
//! @param to_cut Rect being cut down, modified in place.
//! @param salt Mixed into the ID of the result, must differ between cuts of the same rect.
//! @see_also r_salt r_stack
//! @concepts rect salting
//@function r_cut_px

//! A typical application layout: menu, status and side bar are cut from the window, `main` is what remains.
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

//! Bit flags, combine one vertical (`R_TOP`, `R_V_CENTER`, `R_BOTTOM`) and one horizontal (`R_LEFT`, `R_H_CENTER`, `R_RIGHT`) flag. A missing axis means centered. Functions that only need a side, like @ref r_cut_px or @ref r_stack, take a single flag.
//@enum r_alignment

//! Returns the smallest rectangle containing both `a` and `b` with the ID of `a`, `rect_expand_pt` does the same for a point.
//! @see_also rect_intersect r_grow
//! @concepts rect
//@function rect_expand

//! Returns the overlap of `a` and `b` with the ID of `a`, its size is 0 if they don't overlap.
//! @see_also rect_expand
//! @concepts rect
//@function rect_intersect

//! Like @ref r_cut_px, but cuts `fraction` (0 to 1) of the current width or height of `to_cut`.
//! @concepts rect salting
//@function r_cut_fraction

//! Cuts one rectangle per entry of `r` from `to_cut`, sized by each entry's current width or height.
//! @see_also r_cut_px r_expand_group
//! @concepts rect salting
//@function r_cut_group

//! Grows `to_expand` by `pixels` towards `dir` and returns the added area.
//! @see_also r_cut_px r_expand_group
//! @concepts rect
//@function r_expand_px

//! Grows `to_expand` once per entry of `r`, sized by each entry's current width or height, and stores the added areas in `r`.
//! @see_also r_expand_px r_cut_group
//! @concepts rect
//@function r_expand_group

//! Moves a rectangle by the given offset, keeping its ID.
//! @see_also r_grow r_aligned
//! @concepts rect
//@function r_offset

//! Grows a rectangle by `pixels` on all sides, or per side with `r_grow_sides`, keeping its ID.
//! @see_also r_shrink r_offset
//! @concepts rect
//@function r_grow

//! Shrinks a rectangle by `pixels` on all sides, or per side with `r_shrink_sides`, keeping its ID.
//! @see_also r_grow r_inner
//! @concepts rect
//@function r_shrink

//! Enlarges the rectangle to at least `w` by `h` pixels.
//! @see_also r_fill
//! @concepts rect
//@function r_min_size

//! Distributes the free space of `target` evenly among the rects in `r` and places them next to each other along `dir`. `R_LEFT`/`R_RIGHT` arranges them horizontally, `R_TOP`/`R_BOTTOM` vertically, `R_RIGHT`/`R_BOTTOM` start at the far end. On the other axis each rect is stretched to fill `target`.
//!
//! ```c
//! rect name = { .w = 80 }, value = { .w = 40 };
//! r_fill(R_LEFT, row, (rect*[]){ &name, &value }, 2);
//! ```
//! 
//! The rectangles should be initialized with their minimum sizes before calling.
//! @concepts rect alignment
//@function r_fill

//@section Salting
//! Every rect carries a 64 bit `id`. rcut uses it to recognize elements across rebuilds: hover, focus and pressed state as well as widget data like a text cursor are looked up by ID. IDs must be unique within a UI list and stable between rebuilds.
//!
//! You rarely assign IDs by hand. Every function that derives a rect from another also derives the new ID from the old one, e.g. @ref r_cut_px, @ref r_aligned or @ref r_next. Salting is the explicit form of this: mixing a number into an ID to get a different but reproducible ID.

//! Predefined salt values for @ref r_salt and the cutting functions. You can use these freely or use your own values. There are no "bad" salts other than reusing the same salt at multiple places.
//! @see_also r_salt r_cut_px
//@enum r_salts

//! Returns `r` with a new ID derived from `r`'s @ref rect.id and `salt`. Use it when deriving several elements from the same rect, e.g. two buttons aligned in the same area, which would otherwise get the same ID.
//!
//! Some salts you can use freely (`R_SALT00` to `R_SALT20`). Don't worry any salt will do, you can also just write down a random number.
//! @see_also r_cut_px
//! @concepts salting rect
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

//@section Alignment
//! Alignment functions position a rect relative to another without changing its size. `r_align` and `r_aside` modify the rect in place, `r_aligned` returns a new one. The `_fraction` variants take the position as fractions (0 = left/top, 1 = right/bottom) instead of @ref r_alignment flags, the `_group` variants move several rects by the same offset as the first.

//! Moves `r` into the given corner or edge of `target`, keeping its size. The new ID is derived from both rects.
//! @concepts alignment rect
//@function r_align

//! Returns `src` positioned inside `target` like @ref r_align. Handy for one-off rects: `r_aligned(R_CENTER, area, (rect){ .w = 100, .h = 32 })`.
//! @concepts alignment rect
//@function r_aligned

//! Places `r` outside of `target`, touching the given side and centered on the other axis: `R_RIGHT` puts it right of `target`. For corners the vertical side wins, `R_TOP | R_RIGHT` puts it above `target`, right aligned.
//! @concepts alignment rect
//@function r_aside

//! Offsets `r` by the position of a corner of `target`, treating @ref rect.x and @ref rect.y of `r` as relative to that corner. `R_TOP_LEFT` makes `r` relative to `target`'s origin.
//! @concepts alignment rect
//@function r_relative_to

//@section Layout iterators
//! Layout iterators hand out consecutive rects from an area: each @ref r_next cuts the next one off in the iterator's direction, with spacing in between. They are plain structs on the stack and can be nested freely by using a rect of one iterator as the area of another.
//!
//! Stack layouts (@ref r_stack, @ref r_layout) cut in one direction, flow layouts (@ref r_flow_begin) place elements left to right and wrap. IDs of the returned rects are derived from the start area's ID and the element index.

//! `area` is the space left, `size` and `spacing` are the defaults used by @ref r_next, both can be changed at any time. `last` is the rect returned last. The remaining fields are internal.
//@struct r_layout_iter

//! How a layout iterator places its rectangles.
//@enum r_layout_type

//! Starts a stack layout in `area` cutting from the `dir` side: `R_TOP` stacks downwards, `R_LEFT` left to right, `R_BOTTOM` and `R_RIGHT` start at the opposite ends. Default size and spacing come from the fallback style, use @ref r_layout for those of a specific style.
//! @see_also r_next r_remaining
//! @concepts layout_iter
//@function r_stack

//@example screenshot
static void doc_r_stack(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_button(ui, r_next(&it, -1), "New");
	r_button(ui, r_next(&it, -1), "Open");
	r_button(ui, r_next(&it, -1), "Save");
}

//! Returns the next `size` pixels of the layout, preceded by the iterator's spacing unless it's the first element. A negative `size` uses the default size.
//!
//! - `r_next_fraction`: a fraction of the remaining space, `1.0f / (n - i)` splits it evenly among `n` elements.
//! - `r_next_fit`, `r_next_fit_ex`: just large enough for the text (and icon) in the given style.
//! - `r_next_keyed`: derives the ID from `key` instead of the element index, keeping IDs stable when elements are inserted or removed.
//! @concepts layout_iter
//@function r_next

//! Three equally sized columns using `r_next_fraction`.
//@example screenshot
static void doc_r_next(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter cols = r_stack(ui, r_shrink(ui->root_area, 12), R_LEFT);
	for(int i = 0; i < 3; i++)
		r_button(ui, r_next_fraction(&cols, 1.0f / (3 - i)), (char*[]){ "Low", "Medium", "High" }[i]);
}

//! Returns all space left in the layout iterator.
//! @see_also r_next r_stack
//! @concepts layout_iter
//@function r_remaining

//! Returns the rectangle the layout iterator returned last, to place several elements on the same rectangle.
//! @concepts layout_iter
//@function r_last

//! Like @ref r_stack, but default size and spacing come from the theme for `style`, e.g. `R_STYLE_BUTTON` for the theme's button height. `r_layout_rows` and `r_layout_columns` are shorthands for `R_TOP` and `R_LEFT`.
//! @concepts layout_iter drawable
//@function r_layout

//! Skips `size` pixels of the layout iterator's remaining area.
//! @see_also r_next r_stack
//! @concepts layout_iter
//@function r_gap

//! Takes the next `size` pixels of `s` and returns a stack layout inside it cutting from `dir`, inheriting size and spacing. Typically used to place elements side by side within a vertical layout.
//! @concepts layout_iter
//@function r_row

//! Starts a nested vertical layout in the remaining space of `parent`. If `style` is non-zero a background of that style is added and the content is placed inside its padding. The group's size is determined by @ref r_group_end.
//! @concepts layout_iter drawable
//@function r_group_begin

//! Ends the group, takes the space it used from `parent` and returns that rect. The background added by @ref r_group_begin is resized to fit the content.
//! @concepts layout_iter drawable
//@function r_group_end

//@section Rich text
//! <TODO: Explain rich text>

//! Starts a flow layout inside `parent`: elements are placed left to right and wrap into new lines.
//! The horizontal bits of `align` align each line, the vertical bits align the elements within a line.
//! @concepts layout_iter alignment rich_text
//@function r_flow_begin

//! Adds word-wrapped text to a flow layout, split into one label per line, using the flow's style if `style` is 0.
//! @concepts layout_iter drawable rich_text
//@function r_text

//@example screenshot
static void doc_r_text(r_sdl_frontend* f, r_ui_list* ui)
{
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_layout_iter flow = r_flow_begin(&it, R_STYLE_LABEL, R_TOP_LEFT);
	r_text(&flow, 0, "Text wraps at word boundaries to fit the width of the flow layout.");
	r_flow_end(&flow, &it);
}

//! Finishes the last line and takes the used height from `parent` like @ref r_group_end. Returns the rect the flow occupies.
//! @concepts layout_iter rich_text
//@function r_flow_end

//@section UI lists
//! A UI list (@ref r_ui_list) is a flat list of @ref r_drawables and @ref r_interactables, everything rcut shows lives in one. Building UI means appending to a list: there is no widget tree and there are no widget objects, a button is a drawable plus an interactable with the same rect.
//!
//! Lists are rebuilt as a whole, see @ref r_list_clear. Each window has a root list (@ref r_sdl_frontend.root in rcut_sdl), popups, tab pages and scroll areas are child lists, see @ref Ephemerals.

//! Fields relevant when building UI:
//!
//! - `root_area`: the area the list fills, `0, 0, width, height` for a window.
//! - `document_area`: content area of tab pages and scroll areas.
//! - `resized`: set when the window was resized, cleared by @ref r_list_clear.
//! - `userptr`: free for application use.
//!
//! The remaining fields are internal.
//! @see_also r_create_ui_list r_list_clear
//@struct r_ui_list

//! Maximum number of drawables and interactables of a @ref r_ui_list: 65535 on 64 bit and 4096 on 32 bit systems. The memory is only reserved up front and committed when needed, so a high limit mostly costs address space.
//@define RCUT_LIST_MAX_ELEMENTS

//! Creates a new UI list, it continues to exist until deleted by @ref r_destroy_ui_list.
//! 
//! UI lists store everything in rcut, in particular @ref r_interactables and @ref r_drawables.
//! Each UI list can be cleared and regenerated independently of each other.
//! 
//! `target` is a opaque target ID, usually referring to a window or render target. All coordinates are within that targets space.
//! With rcut_sdl target 0 is the main window, see @ref r_sdl_dialog for more windows.
//!
//! `r_create_ui_list_ex` takes explicit capacities instead of `RCUT_LIST_MAX_ELEMENTS`.
//! @note Memory is only reserved, not commited, as such there is no reason to use the _ex variant unless you are working on systems with small address spaces or processors without a MMU.
//! @see_also r_destroy_ui_list r_popup r_viewport r_scroll_begin
//! @concepts ui_list target
//@function r_create_ui_list

//@example screenshot
static void doc_r_create_ui_list(r_sdl_frontend* f, r_ui_list* ui)
{
	r_ui_list* l = r_create_ui_list_ex(&f->context, 0, 100, 320, 80, 16, 4);
	r_label(l, r_shrink(l->root_area, 16), "Hello from a second list");
}

//! Destroys the target UI List.
//!
//! @note You only need to destroy lists created using @ref r_create_ui_list. All other functions that create UI lists are "ephemeral", meaning they automatically get garbage collected if they are not renewed in the next two rebuilds.
//! @see_also r_create_ui_list r_list_set_visibility r_sdl_close_requested r_ephemeral_create
//! @concepts ui_list ephemeral
//@function r_destroy_ui_list

//@example
static void doc_r_destroy_ui_list(r_sdl_frontend* f, r_ui_list* ui)
{
	r_ui_list* dialog = r_sdl_dialog(f, 200, 320, 160, "Settings");
	if(r_sdl_close_requested(f, dialog, true))
		r_destroy_ui_list(dialog);
}

//! Shows or hides the list, a hidden list and its child lists are not drawn and receive no input.
//! @see_also r_list_is_visible r_create_ui_list
//! @concepts ui_list
//@function r_list_set_visibility

//! Returns the bounding rectangle of all drawables in the list, in list space.
//! @see_also r_scroll_end rect_expand
//! @concepts ui_list drawable
//@function r_list_get_content_rect

//! Returns false if the list or any of its parent lists is hidden.
//! @see_also r_list_set_visibility
//! @concepts ui_list
//@function r_list_is_visible

//! Prints the list's drawables and interactables to stdout for debugging.
//! @see_also r_ui_list
//! @concepts ui_list drawable interactable
//@function r_list_print

//@section Rebuilding
//! rcut is not immediate mode: lists are only rebuilt when the UI's structure changes, e.g. after a resize (@ref r_ui_list.resized) or when elements are added or removed. Input and changes to bound values (@ref R_PTR) are handled without a rebuild.
//!
//! A rebuild is @ref r_list_clear followed by the building code. Focus, hover and widget state like a text cursor survive as long as elements keep their IDs.

//! Starts a rebuild: the current contents become the previous frame and the list is emptied. The previous frame is kept until the next clear, so state can be carried over by ID, see @ref r_list_find_last_frame_interact.
//! @concepts rebuild ui_list
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

//! Updates focus, hover and drag state to the list's current contents, e.g. moves focus away from an element that no longer exists. Done automatically on input and visibility changes, only call it if you need that state to be correct right after a rebuild.
//! @concepts rebuild ui_list interactable
//@function r_list_reconcile

//@section Targets
//! A target is what a root list is drawn into, usually a window. It's an opaque `uint64_t` whose meaning is up to the frontend, rcut_sdl uses window indices with 0 being the main window. Child lists share their parent's coordinate system, but popups may be shown on a different target such as a window of their own.

//! Returns the list's origin in target space, scrolling included, and the rect it is clipped to. For root lists this is `root_area`, child lists are offset by their parents.
//! @concepts target ui_list
//@function r_list_get_transform

//@section Ephemerals
//! Ephemerals are child lists with automatic lifetime: they are created while building their parent and destroyed when the parent is rebuilt without creating them again. Popups, tooltips, tab pages and scroll areas are ephemerals. They are identified by an ID, so the same popup is found again across rebuilds.

//! Maximum number of elements of a tooltip list, all other lists use @ref RCUT_LIST_MAX_ELEMENTS.
//! @see_also RCUT_LIST_MAX_ELEMENTS r_tooltip r_popup
//@define RCUT_EPHEMERAL_MAX_ELEMENTS

//! Ephemerals are UI lists with automatic lifetime managment, garbage collected unless created or kept alive during a rebuild of their parent.
//!
//! `id` is the lookup key, salt the anchor with @ref r_salt to allow multiple ephemerals per item: `r_salt(anchor, P400).id`. `anchor`'s @ref rect.id is stored as `anchor_id`.
//!
//! - `r_ephemeral_create`: creates the list, or revives and clears an existing one.
//! - `r_ephemeral_create_ex`: explicit capacities like @ref r_create_ui_list_ex, only applied on creation.
//! - `r_ephemeral_create_or_find`: returns an existing list unchanged, otherwise creates it.
//! - `r_ephemeral_find`: NULL if missing or destroyed, doesn't keep alive. Close with `r_destroy_ui_list(r_ephemeral_find(parent, id))`.
//! - `r_ephemeral_keep_alive`: like find, but keeps alive.
//! @see_also r_popup r_viewport r_destroy_ui_list
//! @concepts ephemeral ui_list salting
//@function r_ephemeral_create

//! Returns the popup with the given ID, creating it if necessary, an existing popup is returned unchanged. Its `root_area` is `anchor`, position it with @ref r_popup_place. If the frontend supports it the popup gets a window of its own.
//! @concepts ephemeral ui_list
//@function r_popup

//! `R_POPUP_TOOLTIP` for display only popups, `R_POPUP_AUTOCLOSE` for menus closing on outside clicks or Escape. 0 for a popup that stays open until destroyed.
//@enum r_popup_flags

//! Creates a child list drawn inside `anchor` of its parent and clipped to it, its contents use coordinates relative to `anchor`. It is cleared on every call, build its contents afterwards. Used for scroll areas and tab pages.
//! @concepts ephemeral ui_list
//@function r_viewport

//! Returns a viewport for `r` with room for a vertical scrollbar on the right. Build the scrollable content into it, then call @ref r_scroll_end. `r_scroll_begin_ex` additionally draws a frame of `frame_style` around it.
//!
//! ```c
//! r_ui_list* v = r_scroll_begin(ui, area);
//! r_layout_iter it = r_stack(v, (rect){ 0, 0, v->root_area.w, 100000, v->root_area.id }, R_TOP);
//! for(int i = 0; i < 100; i++)
//! 	r_label(v, r_next(&it, 24), "Line");
//! r_scroll_end(ui, v);
//! ```
//! @concepts ephemeral ui_list
//@function r_scroll_begin

//! Computes the content height from the viewport's drawables, clamps the scroll position and adds the scrollbar and mouse wheel handling to `parent`.
//! @concepts ephemeral ui_list
//@function r_scroll_end

//! Scrolls so that `x`, `y` in content coordinates is the top left corner of the visible area, clamped to the content size.
//! @concepts ephemeral ui_list
//@function r_scroll_to

//! Returns a `w` by `h` rect next to `anchor`: below it, or right of it with `R_RIGHT`. Flips to the opposite side if there's not enough room and is clamped to the window.
//! @concepts ephemeral alignment
//@function r_popup_place

//! Closes popups opened with `R_POPUP_AUTOCLOSE`, only the topmost one if `only_topmost` is set. Returns true if anything was closed.
//! @concepts ephemeral
//@function r_close_popups

//@section Drawables
//! A drawable (@ref r_drawable) is a rect with a style and optionally text, an icon or an image. Drawables are pure data, the renderer (rcut_draw or your own) decides how a style looks. They are drawn in the order they were added. High level functions like @ref r_button add them for you, @ref r_style and @ref r_labeled add them directly.

//! - `r`, `style`: where and how to draw.
//! - `text`, `icon`: label content. Text is not copied, it must stay valid until the next rebuild.
//! - `parent_id`: inherit hover, pressed and focus state from the element with this ID, used for parts of widgets.
//! - `outer_style`: style of the enclosing element, e.g. of the button a label is in.
//! - `image_data`: image drawn inside the stylebox.
//! - `dynamic`: bound value the drawable follows without a rebuild.
//@struct r_drawable

//! Adds a drawable of `style` covering `r` and returns it to set further fields. The basic building block for custom widgets.
//! @concepts drawable
//@function r_style

//! Adds a drawable of `style` with `text` placed inside its padding, like a button without interaction.
//! @concepts drawable
//@function r_labeled

//! Returns the drawable with the given ID from the current frame, or NULL.
//! @see_also r_list_find_last_frame_drawable r_drawable
//! @concepts drawable rebuild
//@function r_list_find_this_frame_drawable

//! Returns the drawable with the given ID from before the last @ref r_list_clear, or NULL.
//! @concepts drawable rebuild
//@function r_list_find_last_frame_drawable

//! How a dynamic drawable follows its binding: `R_DYN_ALIGNED` moves it (slider thumbs), `R_DYN_FILLED` resizes it (progress bars), `R_DYN_TOGGLED` adds a state while the binding matches (checkboxes), `R_DYN_CUSTOM` calls an update function.
//@enum r_dynamic_rect_kind

//! Binding and parameters of a dynamic drawable, set up by `r_drawable_thumb`, `r_drawable_fill`, `r_drawable_toggled` and `r_drawable_custom`. `drawn` is the bound value at the last draw.
//@struct r_dynamic_rect_data

//! Returns true and the area to redraw in `damage` if the drawable's bound value changed since it was last drawn or its image needs uploading. Used by renderers to only redraw what changed.
//! @concepts drawable damage
//@function r_drawable_take_change

//! Returns the content rectangle of an element with the style, given its outer rectangle.
//! @see_also r_outer r_draw_stylebox_inner_rect
//! @concepts drawable rect
//@function r_inner

//! Returns the outer rectangle an element with the style needs to fit the content rectangle `inner`.
//! @see_also r_inner r_measure r_draw_stylebox_outer_rect
//! @concepts drawable rect
//@function r_outer

//! Grows the outer rectangle by half the style's default spacing on every side.
//! @see_also r_remove_margins r_layout_default_spacing
//! @concepts drawable rect
//@function r_with_margins

//! Removes the margins added by @ref r_with_margins.
//! @see_also r_with_margins
//! @concepts drawable rect
//@function r_remove_margins

//! Returns the theme's default element size for the style, 28 if the frontend provides none.
//! @see_also r_layout_default_spacing r_theme_layout r_interface
//! @concepts drawable theming
//@function r_layout_default_size

//! Returns the theme's default spacing between elements of the style, 8 if the frontend provides none.
//! @see_also r_layout_default_size r_theme_layout
//! @concepts drawable theming
//@function r_layout_default_spacing

//! Returns the size of `label` in `style`, the content without padding. `r_measure_n` measures the first `len` bytes, `r_measure_ex` includes an icon.
//! @concepts drawable font
//@function r_measure

//! Measures `label` and returns three sizes: `inner` is the content, `outer` includes padding (@ref r_outer) and `outer_with_margins` also includes the default spacing (@ref r_with_margins), it may be NULL.
//! @concepts drawable font
//@function r_style_measure_all

//@section Interactables
//! An interactable (@ref r_interact) is a rect receiving input, callbacks are attached to it per event type. Interactables are identified by their rect's ID: using the same rect twice returns the same interactable, which is how widgets combine several callbacks.

//! - `r`: the hit area.
//! - `cursor`: mouse cursor while hovered.
//! - `traps_tab_focus`, `traps_alt_key`, `traps_arrow_focus`: keep Tab, Alt or arrow keys from moving focus while focused, e.g. for text inputs.
//! - `accelerator_key`: Alt + this key activates the element.
//! - `press_group`: see @ref r_set_press_group.
//! - `next_focus`, `previous_focus`, `directional_focus`: override keyboard navigation, NULL for automatic.
//! - `callbacks`: linked list of @ref r_callbacks.
//@struct r_interact

//! Returns the interactable with `r`'s ID, creating it if the current build has none yet. `r_interactable_new` always creates one, allowing several hit areas with the same ID, like the buttons of a spin box.
//! @concepts interactable
//@function r_interactable

//! Sets the mouse capture group of the interactable at `r`. While an element is pressed only it and elements of its group receive mouse events. Menus use this so the mouse can be dragged from a menu button onto an item.
//! @concepts interactable input
//@function r_set_press_group

//! Returns the visible interactable with the given ID, or NULL.
//! @see_also r_list_find_this_frame_interact r_interact
//! @concepts interactable
//@function r_find_interact

//! Returns the interactable with the given ID from the current frame, or NULL.
//! @see_also r_list_find_last_frame_interact r_find_interact
//! @concepts interactable rebuild
//@function r_list_find_this_frame_interact

//! Returns the interactable with the given ID from before the last @ref r_list_clear, or NULL.
//! @concepts interactable rebuild
//@function r_list_find_last_frame_interact

//! - `event`: the event type it handles.
//! - `function`: returns true if it handled the event.
//! - `data`, `size`: data owned by the list, see `r_callback_set_data` and `r_copy_last_frame_data`.
//! - `menu_label`, `hotkey`, `hotkey_modifier`: for `R_CONTEXT` callbacks, shown in the context menu.
//! - `parent`: the owning interactable.
//@struct r_callback

//! Returns true if the element should look pressed: it is pressed and the mouse is still over it.
//! @see_also r_draw_take_damage
//! @concepts interactable drawable
//@function r_is_pressed_visual

//! Moves the keyboard focus to `new_focus`, or clears it if NULL, and sends `R_FOCUS_EXIT` and `R_FOCUS_ENTER`.
//! @see_also r_event_type r_focus_wants_text
//! @concepts interactable input
//@function r_change_focus

//! Returns true if the focused element accepts text input.
//! @see_also r_post_text r_change_focus
//! @concepts interactable input
//@function r_focus_wants_text

//! Returns the mouse cursor of the hovered or dragged element.
//! @see_also r_cursor
//! @concepts interactable input frontend
//@function r_hover_cursor

//! Mouse cursor shapes, see @ref r_hover_cursor.
//! @see_also r_hover_cursor r_interact
//@enum r_cursor

//@section Data binding
//! Widgets don't store values, they read and write them through a binding (@ref r_data_binding). A binding either points to a variable (@ref R_PTR, @ref R_BITFIELD) or wraps a getter and setter (@ref R_GETSET). Bound values are read when drawing, so changes show up without a rebuild.

//! Passed by value. Without `use_function_indirection` it refers to a variable through `ptr`, otherwise to a getter and setter. Create it with @ref R_PTR, @ref R_BITFIELD or @ref R_GETSET.
//@struct r_data_binding

//! Binds the variable `P` points to. The type is detected with `_Generic`: `bool`, any integer type, `float` and `double` are supported.
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

//! Binds through functions, taking designated initializers for @ref r_data_desc. Use it to convert units or to react to changes.
//!
//! ```c
//! static float get_gain(void);
//! static void  set_gain(float gain);
//! // [...]
//! r_slider_h(ui, r, 0, 1, 0.01f, R_GETSET(.getter_f32 = get_gain, .setter_f32 = set_gain));
//! ```
//@define R_GETSET

//! Fields for @ref R_GETSET, set one getter and one setter. Names are `getter_TYPE` and `setter_TYPE` with TYPE being `b`, `i32`, `i64`, `u32`, `u64`, `f32` or `f64`. Variants with `i32`/`u64` pass `index` and with `ptr` pass `pointer` as additional argument.
//@struct r_data_desc

//! Type of the value a pointer binding refers to.
//! @see_also R_PTR r_data_binding
//@enum r_data_type

//! Writes an integer to the binding, converted to the bound type.
//! @see_also r_geti r_setd r_data_binding
//! @concepts binding
//@function r_seti

//! Reads the binding as an integer.
//! @see_also r_seti r_getd
//! @concepts binding
//@function r_geti

//! Writes a double to the binding, converted to the bound type.
//! @see_also r_getd r_seti
//! @concepts binding
//@function r_setd

//! Reads the binding as a double.
//! @see_also r_setd r_geti
//! @concepts binding
//@function r_getd

//@section UI elements

//! A push button showing `text`. It does nothing by itself, attach an action with @ref r_on_click using the same rect. `r_button_ex` takes a style and icon and returns the drawable.
//! @see_also r_on_click r_icon
//! @concepts drawable
//@function r_button

//@example screenshot
static void doc_r_button(r_sdl_frontend* f, r_ui_list* ui)
{
	rect ok = r_aligned(R_CENTER, ui->root_area, (rect){ .w = 100, .h = 32 });
	r_button(ui, ok, "OK");
	r_on_click(ui, ok, on_ok);
}

//! Draws `text` in `r`. `r_label_ex` takes a style and icon and returns the drawable. `r_label_value` shows a bound value through a printf format with a single conversion and updates without a rebuild.
//! @see_also r_text r_header
//! @concepts drawable binding
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

//! Calls `on_click` when the element at `r` is activated: clicked, Enter while focused or its accelerator key. The `_i32`, `_u64`, `_f32`, `_f64`, `_ptr` and `_ptr_i32` variants pass the given values to `on_click`, so one function can serve many elements.
//! @concepts callback
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

//! A checkbox with `text` next to it, toggling a bool binding.
//! @param binding Bound value, true while checked. Any integer type works, see @ref R_BITFIELD for flags.
//! @concepts binding
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

//! Horizontal (`r_slider_h`) or vertical (`r_slider_v`) slider for values from `min` to `max`. Values snap to multiples of `step` from `min`, 0 for continuous. Arrow keys move by one step, ten with Shift, Home and End jump to the ends.
//! @see_also r_slider_box r_spinbox
//! @concepts binding
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

//! A style is a base style (`R_STYLE_*`) combined with optional flags, states, a variant and a component, e.g. `R_STYLE_PANEL | R_VARIANT_INSET`. States are set by widgets. Variants select alternative looks, components address parts of compound widgets like the thumb of a slider. Themes fall back to simpler combinations if they don't define a specific one.
//@enum r_style

//! Draws a built-in icon (@ref r_icon_variant) centered in `r`. `style` is the style of the enclosing element, 0 for default.
//! @concepts icon
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
//! @concepts drawable
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
//! @concepts drawable
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
//! @concepts drawable
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
//! @concepts binding
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

//! A radio button, checked while the binding equals `value`, clicking it sets the binding to `value`. Use one binding with different values for a group.
//! @concepts binding
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

//! A list item working like @ref r_radio: selected while the binding equals `value`, clicking selects it.
//! @concepts binding
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

//! A compact slider showing `text` and the value. Change it by dragging, with the mouse wheel or the arrow keys. `step` also determines the displayed precision.
//! @concepts binding
//@function r_slider_box

//@example screenshot
static void doc_r_slider_box(r_sdl_frontend* f, r_ui_list* ui)
{
	static float brightness = 40;
	// [...]
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_slider_box(ui, r_next(&it, 26), "Brightness", 0, 100, 1, R_PTR(&brightness));
}

//! A number input with minus and plus buttons, holding a button repeats. The value can also be typed in or changed with the mouse wheel, Shift multiplies the step by ten. Values are clamped to `min` and `max`.
//! @concepts binding
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
//! @concepts binding
//@function r_progress

//@example screenshot
static void doc_r_progress(r_sdl_frontend* f, r_ui_list* ui)
{
	static float progress = 65.0f;
	r_layout_iter it = r_stack(ui, r_shrink(ui->root_area, 12), R_TOP);
	r_label(ui, r_next(&it, 20), "Downloading... 65%");
	r_progress(ui, r_next(&it, 8), 0, 100, R_PTR(&progress));
}

//! A tab bar with `count` tabs and a page for each. `pages` receives one viewport per tab, build each page into its `document_area`. Only the selected page is visible, switching tabs doesn't need a rebuild.
//! @concepts binding ephemeral
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

//! A button showing the selected entry of `items`, clicking it opens a popup to choose another. `selected` is bound to the index. Arrow keys change the selection while focused.
//! @concepts binding ephemeral
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

//! A row of joined buttons sharing the width evenly, one of them selected. `selected` is bound to the index.
//! @concepts binding
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

//! One row of a tree view. `depth` indents the row, `has_children` adds an arrow toggling the `open` binding, double clicking the row does the same. Like @ref r_selectable the row is selected while `selected` equals `value`. Returns true if the node has children and is open, so children are only built when visible. Use @ref r_next_keyed to keep IDs stable when nodes open or close.
//! @concepts binding
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

//! Splits `r` into `first` and `second` with a draggable separator in between, side by side for `R_LEFT`/`R_RIGHT`, stacked otherwise. `fraction` is bound to the size of `first` relative to the whole, `min_first` and `min_second` are minimum sizes in pixels.
//! @concepts binding rect
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

//! A single line text input editing `buffer` in place, `capacity` being the buffer size including the terminator. `placeholder` is shown while empty. Supports selection, clipboard and IME input. `r_line_edit_on_submit` sets a function called on Enter.
//! @concepts drawable interactable
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

//! Like @ref r_line_edit, but multiline: Enter inserts a newline and the content scrolls vertically.
//! @concepts drawable interactable
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

//! The text input behind @ref r_line_edit and @ref r_text_area, with a custom `style` and the `multiline` switch.
//! @concepts drawable
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

//! Caret, selection and scroll position of a text input. Stored as callback data, so it survives rebuilds as long as the input's rect keeps its ID.
//@struct r_text_edit_state opaque

//! Shows `text` (at most 511 bytes) in a tooltip while the mouse hovers `r`.
//! @see_also r_popup RCUT_MOUSE_*
//! @concepts ephemeral
//@function r_tooltip

//@example
static void doc_r_tooltip(r_sdl_frontend* f, r_ui_list* ui)
{
	rect ok = r_aligned(R_CENTER, ui->root_area, (rect){ .w = 100, .h = 32 });
	r_button(ui, ok, "OK");
	r_tooltip(ui, ok, "Applies the settings and closes the window.");
}

//! Clears the dialog list, draws its background and returns the content area inside the padding. Call it first when building a dialog.
//! @concepts ui_list
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

//! Built-in icons for @ref r_icon and the `icon` arguments of widgets. They are drawn by the theme and match the rest of the UI.
//@enum r_icon_variant
