//@module draw

//@section Default theme

//! Loads the built-in theme: clears all style rules and theme styleboxes, then declares and builds rules for every style. `background` and `primary` are the window and accent color, the palette is derived from them, light or dark depending on `background`. 0 selects the default colors and 14 for `font_size`. Customize the result with @ref r_theme or @ref r_theme_tweak.
//! @concepts default_theme
//@function r_theme_default

static void doc_theme_preview(r_ui_list* ui)
{
	static bool grid = true, snap = false, autosave = true, dark = false;
	static int filter = 1, period = 0;
	static char* filters[] = { "Nearest", "Bilinear", "Bicubic" };
	static char* periods[] = { "Day", "Week", "Month" };
	rect area = r_shrink(ui->root_area, 16);
	rect buttons = r_cut_px(R_BOTTOM, &area, 32, R_SALT00);
	r_layout_iter left  = r_stack(ui, r_cut_fraction(R_LEFT, &area, 0.5f, R_SALT01), R_TOP);
	r_layout_iter right = r_stack(ui, area, R_TOP);
	r_checkbox(ui, r_next(&left, -1), "Show grid", R_PTR(&grid));
	r_checkbox(ui, r_next(&left, -1), "Snap to grid", R_PTR(&snap));
	r_toggle(ui, r_next(&left, -1), "Autosave", R_PTR(&autosave));
	r_toggle(ui, r_next(&left, -1), "Dark mode", R_PTR(&dark));
	for(int i = 0; i < 3; i++)
		r_radio(ui, r_next(&right, -1), filters[i], i, R_PTR(&filter));
	r_segmented(ui, r_next(&right, -1), periods, 3, R_PTR(&period));
	r_layout_iter row = r_stack(ui, buttons, R_LEFT);
	r_button_ex(ui, r_next(&row, 120), R_STYLE_BUTTON | R_VARIANT_PRIMARY, R_ICON_CHECKED, "Confirm");
	r_button(ui, r_next(&row, 100), "Cancel");
}

//@example screenshot
static void doc_r_theme_default_crimson(r_sdl_frontend* f, r_ui_list* ui)
{
	// Use default colors.
	r_theme_default(&f->draw, 0, 0, 0);
	//@example_end
	doc_theme_preview(ui);
}

//@example screenshot
static void doc_r_theme_default_navy(r_sdl_frontend* f, r_ui_list* ui)
{
	uint32_t bg_color = r_rgb_u8(17, 23, 35);
	uint32_t fg_color = r_rgb_u8(59, 136, 251);
	r_theme_default(&f->draw, bg_color, fg_color, 0);
	//@example_end
	doc_theme_preview(ui);
}

//@example screenshot
static void doc_r_theme_default_violet(r_sdl_frontend* f, r_ui_list* ui)
{
	uint32_t bg_color = r_rgb_u8(224, 222, 229);
	uint32_t fg_color = r_rgb_u8(120, 88, 246);
	r_theme_default(&f->draw, bg_color, fg_color, 0);
	//@example_end
	doc_theme_preview(ui);
}

//@example screenshot
static void doc_r_theme_default_green(r_sdl_frontend* f, r_ui_list* ui)
{
	uint32_t bg_color = r_rgb_u8(222, 219, 212);
	uint32_t fg_color = r_rgb_u8(46, 124, 108);
	r_theme_default(&f->draw, bg_color, fg_color, 0);
	//@example_end
	doc_theme_preview(ui);
}

//@section DPI scale
//! rcut works in logical pixels: rects, font sizes and theme values are the same on every display. rcut_draw multiplies them with the scale factor when rasterizing, so at 1.5 the UI is 150% large and stays sharp. Styleboxes, glyphs and icons are rasterized for the current scale and cached.

//! Sets the DPI scale factor.
//! This clears the cached stylebox and fonts, so they are recomputed the next time they are rendered, and calls the backend's `scale_changed`.
//! @see_also r_draw_backend r_draw_clear_cache
//! @concepts dpi_scale draw-context
//@function r_draw_set_scale

//! Drops all cached textures (styleboxes, glyphs, icons), they are rasterized again when next drawn. It only makes sense to call this if you want to avoid stale textures in the cache.
//! @concepts dpi_scale font stylebox image draw-context
//@function r_draw_clear_cache

//! Evicts the cache entry for `img`. It will be uploaded again next redraw if used by a drawable.
//! @see_also r_image_buffer
//! @concepts image draw-context
//@function r_image_invalidate

//@section Draw backend
//! Interface to a renderer, all functions receive `userptr`.
//!
//! The cache functions return a cache handle, an opaque uint64. This can be a pointer, an index or even a encoded texel position. rcut_draw only passes these back into the r_draw_backend functions.
//!
//! - `cache_argb`: upload a premultiplied ARGB texture and return to a handle to it's cache entry. `permanent` textures must be kept until invalidated, others may be evicted.
//! - `cache_monochrome`: like cache_argb, but the texture is 8 bit coverage data. 
//! - `cache_image_buffer`: upload an @ref r_image_buffer.
//! - `submit`: draw the quads of a frame in order.
//! - `is_cached`: whether a handle is still valid, evicted textures which are still in use will be queued again before the next `submit` call.
//! - `invalidate_cache`: tells the cache that the content in that handle is out of date and should be evicted.
//! - `scale_changed`: optional, called by @ref r_draw_set_scale.
//!
//! <TODO: r_draw_backend tutorial>
//@struct r_draw_backend

//! A textured rectangle: `src` of texture `handle` drawn to `dst`, tinted with `color`. `transform` maps `dst` to target space and clips it.
//@struct r_draw_quad

//! Affine 2x3 matrix applied to quad positions and the clip rect in target pixels.
//@struct r_draw_transform

//! Rectangle with float coordinates. These are only used by @ref r_draw_quad and passed to the renderer interface that way.
//! @see_also r_draw_quad
//@struct r_frect

//@section Colors

//! Packs 8 bit red, green and blue components into an opaque ARGB color.
//! @see_also r_rgba_u8 r_rgb_f r_hex
//! @concepts colors
//@function r_rgb_u8

//! Packs 8 bit red, green, blue and alpha components into an ARGB color.
//! @see_also r_rgb_u8 r_rgba_f r_hexa
//! @concepts colors
//@function r_rgba_u8

//! Packs red, green and blue components in the range 0 to 1 into an opaque ARGB color.
//! @see_also r_rgba_f r_rgb_u8
//! @concepts colors
//@function r_rgb_f

//! Packs red, green, blue and alpha components in the range 0 to 1 into an ARGB color.
//! @see_also r_rgb_f r_rgba_u8
//! @concepts colors
//@function r_rgba_f

//! Turns a 0xRRGGBB value into an opaque ARGB color.
//! @see_also r_hexa r_rgb_u8
//! @concepts colors
//@function r_hex

//! Turns a 0xRRGGBB value and an alpha in the range 0 to 1 into an ARGB color.
//! @see_also r_hex r_rgba_u8
//! @concepts colors
//@function r_hexa

//@section Icons
//! Icons are small vector drawings, defined as OP codes in a tiny VM, rasterized at the size they are drawn at like glyphs.
//! Operations all assume a 24x24 canvas, so 12.0f, 12.0f is the center.

//@enum r_draw_icon_op

//! Sets icon `slot` to `icon`, this allows you to override built in icons or add your own.
//! 
//! Icons use the same underlying caching system as fonts, caching each "font size" of the icon as needed. The ops array is not copied and must stay valid for when the icon needs to be cached.
//! @concepts icon
//@function r_draw_upload_icon

//@example screenshot below
static void doc_r_draw_upload_icon(r_sdl_frontend* f, r_ui_list* ui)
{
	// The ops are not copied, so they must outlive the call.
	static const float play[]   = { R_WIDTH(0), R_PLINE(3, 7,5, 19,12, 7,19), R_ICON_OP_END };
	static const float pause[]  = { R_WIDTH(0), R_RECT(6,5, 4,14, 1), R_RECT(14,5, 4,14, 1), R_ICON_OP_END };
	static const float stop[]   = { R_WIDTH(0), R_RECT(6,6, 12,12, 2), R_ICON_OP_END };
	static const float record[] = { R_DISC(12,12, 6), R_ICON_OP_END };
	r_draw_upload_icon(&f->draw, R_ICON_USER+0, &(r_draw_icon_def){ play });
	r_draw_upload_icon(&f->draw, R_ICON_USER+1, &(r_draw_icon_def){ pause });
	r_draw_upload_icon(&f->draw, R_ICON_USER+2, &(r_draw_icon_def){ stop });
	r_draw_upload_icon(&f->draw, R_ICON_USER+3, &(r_draw_icon_def){ record });

	r_layout_iter row = r_stack(ui, r_shrink(ui->root_area, 12), R_LEFT);
	r_button_ex(ui, r_next(&row, 90), R_STYLE_BUTTON, R_ICON_USER+0, "Play");
	r_button_ex(ui, r_next(&row, 90), R_STYLE_BUTTON, R_ICON_USER+1, "Pause");
	r_button_ex(ui, r_next(&row, 90), R_STYLE_BUTTON, R_ICON_USER+2, "Stop");
	r_button_ex(ui, r_next(&row, 90), R_STYLE_BUTTON, R_ICON_USER+3, "Record");
}

//! `ops` points to a list of @ref r_draw_icon_op terminated by `R_ICON_OP_END`, usually created with `R_OP`.
//@struct r_draw_icon_def

//! Instructions of the icon language on a 24x24 canvas. The `R_LINE`, `R_PLINE`, `R_ARC`, `R_CIRCLE`, `R_DISC`, `R_RECT` and `R_WIDTH` macros emit them.
//!
//! ```c
//! static r_draw_icon_def check = { R_OP(R_WIDTH(2.5f), R_PLINE(3, 5, 12, 10, 17, 19, 7)) };
//! // [...]
//! r_draw_upload_icon(&f->draw, R_ICON_USER, &check);
//! r_icon(ui, r, 0, R_ICON_USER);
//! ```
//@enum r_draw_icon_op

//! Returns the largest square centered in `outer`.
//! @see_also r_draw_upload_icon r_draw_render_icon
//! @concepts icon rect
//@function r_draw_icon_get_rect

//@section Draw context
//! <TODO: Explain renderer>
//! 
//! @note If you use rcut_sdl then it will handle all of this by itself and you don't need to concern yourself with it.

//! The rcut_draw context. It is needed by many rcut_draw functions.
//!
//! <TODO: Explain renderer ownership, usage>

//! @note The instance that belongs to rcut_sdl can be accessed via the @ref r_sdl_frontend.draw struct member.
//@struct r_draw opaque

//! <TODO: Explain backend and scale>
//! @concepts draw-context dpi_scale
//@function r_draw_init

//! Frees all fonts, caches and memory owned by the renderer.
//! @see_also r_draw_init
//! @concepts draw-context
//@function r_draw_free

//! <TODO: Explain rendering a target>
//! @concepts draw-context ui_list target
//@function r_draw_render

//! <TODO: Explain partial rendering>
//! @concepts draw-context damage
//@function r_draw_render_area

//! <TODO: Explain damage rect usage>
//! @concepts draw-context damage
//@function r_draw_take_damage

//! <TODO: Explain when bounds are needed>
//! @concepts draw-context target
//@function r_draw_target_bounds

//! <TODO: Explain software backend usage>
//@struct r_draw_software opaque

//! <TODO: Explain cache size>
//! @concepts draw-context
//@function r_draw_software_init

//! Frees the buffers owned by the software renderer.
//! @see_also r_draw_software_init
//! @concepts draw-context
//@function r_draw_software_free

//! <TODO: Explain backend creation>
//! @concepts draw-context draw_backend
//@function r_draw_software_backend

//! <TODO: Describe target pixel buffer>
//! @concepts draw-context target
//@function r_draw_software_set_target

//@section Draw functions
//! <TODO: Explain draw functions>

//! <TODO: Explain render pass usage>
//! @concepts draw-function
//@function r_draw_render_pass_begin

//! <TODO: Explain render pass end>
//! @concepts draw-function
//@function r_draw_render_pass_end

//! <TODO: Describe drawable rendering>
//! @concepts draw-function drawable
//@function r_draw_render_drawable

//! Draws an icon centered in `bounds` in the current render pass.
//! @see_also r_draw_render_pass_begin r_draw_upload_icon
//! @concepts draw-function icon
//@function r_draw_render_icon

//! Draws text at the top left of `bounds` in the current render pass.
//! @see_also r_draw_render_pass_begin r_draw_measure_text
//! @concepts draw-function font
//@function r_draw_render_text

//! Draws the stylebox in slot `stylebox` stretched to `outer` in the current render pass.
//! @see_also r_draw_render_pass_begin r_draw_generate_stylebox
//! @concepts draw-function stylebox
//@function r_draw_render_stylebox

//! Draws a solid rectangle in the current render pass.
//! @see_also r_draw_render_pass_begin
//! @concepts draw-function
//@function r_draw_render_fill

//@section Theming
//! <TODO: Explain theme>

//! Sets the label color, `r_theme_label_color` uses it exactly as given while `r_theme_label_color_auto` adapts it to stay readable on the element's background.
//! @see_also r_theme_label r_theme_surface
//! @concepts theming
//@function r_theme_label_color

//! <TODO: Explain theme rule builder>
//! @concepts theming drawable
//@function r_theme

//! <TODO: Explain tweaking existing rules>
//! @concepts theming drawable
//@function r_theme_tweak

//! <TODO: Explain build variants, states>
//! @concepts theming style_rule stylebox
//@function r_theme_build

//! Fills the background with a vertical gradient from `top` to `bottom`.
//! @see_also r_theme_surface r_theme_tint
//! @concepts theming
//@function r_theme_fill

//! Sets the color assumed behind a translucent background, used to pick readable label colors.
//! @see_also r_theme_fill r_theme_label_color
//! @concepts theming
//@function r_theme_surface

//! Blends `color` over the background and border colors set so far.
//! @see_also r_theme_fill r_theme_border
//! @concepts theming
//@function r_theme_tint

//! Sets the border width and color, `r_theme_border_color` only changes the color.
//! @see_also r_theme_underline r_theme_bevel r_theme_border_split
//! @concepts theming
//@function r_theme_border

//! Replaces the border with a bottom border of the given width and color.
//! @see_also r_theme_border
//! @concepts theming
//@function r_theme_underline

//! Sets separate top and bottom border colors.
//! @see_also r_theme_border r_theme_border_split
//! @concepts theming
//@function r_theme_bevel

//! Sets the border width with separate top-left and bottom-right colors, for sunken or raised edges.
//! @concepts theming
//@function r_theme_border_split

//! Sets the radius of all corners.
//! @see_also r_theme_corners
//! @concepts theming
//@function r_theme_radius

//! Sets the radius of each corner individually.
//! @see_also r_theme_radius
//! @concepts theming
//@function r_theme_corners

//! Sets the horizontal and vertical content padding.
//! @see_also r_theme_layout r_inner
//! @concepts theming
//@function r_theme_padding

//! Sets a drop shadow with vertical offset, blur radius and opacity.
//! @see_also r_theme_shadow2 r_theme_no_shadow r_theme_inset
//! @concepts theming
//@function r_theme_shadow

//! Sets a second drop shadow, drawn in addition to the one from @ref r_theme_shadow.
//! @see_also r_theme_shadow r_theme_glow
//! @concepts theming
//@function r_theme_shadow2

//! Removes both drop shadows.
//! @see_also r_theme_shadow r_theme_shadow2
//! @concepts theming
//@function r_theme_no_shadow

//! Sets an inner shadow with vertical offset, blur radius and opacity.
//! @see_also r_theme_shadow
//! @concepts theming
//@function r_theme_inset

//! Adds a glow of the given color and width, using the second shadow if the first one is set.
//! @see_also r_theme_shadow r_theme_shadow2
//! @concepts theming
//@function r_theme_glow

//! Sets the label color, adapted to stay readable, and the label alignment.
//! @see_also r_theme_label_color r_theme_font
//! @concepts theming alignment
//@function r_theme_label

//! Sets the label font.
//! @see_also r_theme_label r_draw_get_font
//! @concepts theming font
//@function r_theme_font

//! Sets the icon color and size.
//! @see_also r_icon
//! @concepts theming icon
//@function r_theme_icon

//! Sets the default size and spacing returned by @ref r_layout_default_size and @ref r_layout_default_spacing.
//! @see_also r_layout_default_size r_layout_default_spacing r_theme_padding
//! @concepts theming drawable
//@function r_theme_layout

//! Enables or disables the focus ring.
//! @concepts theming interactable
//@function r_theme_focus

//! Focus indicator styles for the `r_theme_focus` function.
//@enum r_theme_focus

//! Draws labels with an outline of the given width and color.
//! @see_also r_theme_text_shadow
//! @concepts theming font
//@function r_theme_text_outline

//! Draws labels with a shadow of the given offset, blur and color.
//! @see_also r_theme_text_outline r_theme_shadow
//! @concepts theming font
//@function r_theme_text_shadow

//@section Style rules
//! <TODO: Describe style rule fields>
//@struct r_draw_style_rule

//! Returns the style rule for exactly `key`, or NULL.
//! @see_also r_draw_set_style_rule r_draw_find_style_rule r_draw_style_rule
//! @concepts style_rule drawable
//@function r_draw_get_style_rule

//! <TODO: Explain style fallback lookup>
//! @concepts style_rule drawable
//@function r_draw_find_style_rule

//! Copies `rule` as the style rule for `key`, replacing any existing one.
//! @see_also r_draw_get_style_rule r_draw_delete_style_rule
//! @concepts style_rule drawable
//@function r_draw_set_style_rule

//! Removes the style rule for `key`.
//! @see_also r_draw_set_style_rule
//! @concepts style_rule drawable
//@function r_draw_delete_style_rule

//@section Styleboxes
//! <TODO: Explain styleboxes>

//! <TODO: Describe slicing parameters>
//! 
//! `r_draw_upload_stylebox_fill`: Upload a aspect ratio aware fill to be used with the stylebox. It will be drawn under the rest of the stylebox. (The center of the stylebox must be transparent.)
//! @concepts stylebox
//@function r_draw_upload_stylebox

//! Given the outer rect, calculates the inner rect (e.g. where text should be positioned).
//! @see_also r_draw_stylebox_outer_rect r_inner
//! @concepts stylebox rect
//@function r_draw_stylebox_inner_rect

//! Given the inner rect, calculates the outer rect (e.g. how big the entire drawable needs to be to contain the contents of inner).
//! @see_also r_draw_stylebox_inner_rect r_outer
//! @concepts stylebox rect
//@function r_draw_stylebox_outer_rect

//! <TODO: Describe stylebox definition>
//@struct r_draw_stylebox_def

//! <TODO: Describe gradient fields>
//@struct r_draw_gradient

//! <TODO: Explain generated styleboxes>
//! @concepts stylebox
//@function r_draw_generate_stylebox

//! Frees the stylebox slot and its cached textures.
//! @see_also r_draw_generate_stylebox r_draw_upload_stylebox
//! @concepts stylebox
//@function r_draw_free_stylebox

//! Sets a solid border of the given color, thickness and corner radius on all sides.
//! @see_also r_draw_stylebox_def r_draw_generate_stylebox
//! @concepts stylebox
//@function r_stylebox_set_outline_border

//! <TODO: Describe shadow helper>
//! @concepts stylebox
//@function r_stylebox_set_shadow

//! Sets a solid background color.
//! @see_also r_stylebox_set_bg_vertical_gradient r_draw_stylebox_def
//! @concepts stylebox
//@function r_stylebox_set_bg_color

//! <TODO: Describe gradient helper>
//! @concepts stylebox
//@function r_stylebox_set_bg_vertical_gradient

//! <TODO: Describe raised border helper>
//! @concepts stylebox
//@function r_stylebox_set_raised_borders_simple

//! Number of stylebox slots.
//! @see_also RCUT_DRAW_USER_STYLEBOXES r_draw_generate_stylebox
//@define RCUT_DRAW_MAX_STYLEBOXES

//! First stylebox slot for the application, themes only use the slots below.
//! @see_also RCUT_DRAW_MAX_STYLEBOXES r_draw_upload_stylebox
//@define RCUT_DRAW_USER_STYLEBOXES

//@section Fonts
//! <TODO: Explain fonts>

//! Selects the text backend: `RCUT_USE_HARFBUZZ`, `RCUT_USE_HB_MINI` or `RCUT_USE_STB_TRUETYPE`, if none is defined the backend is detected from previously included headers.
//! @see_also r_draw_font_register
//@define RCUT_USE_*

//! <TODO: Describe return rect>
//! 
//! Widths are measured to the nearest pixel, so text may overhang its measured width by up to half a pixel (+1 device pixel of truncation).
//! @concepts font
//@function r_draw_measure_text

//! <TODO: Explain font handle usage>
//@struct r_draw_font opaque

//! <TODO: Describe weight and style flags>
//@enum r_draw_font_flags

//! <TODO: Explain system font discovery>
//! @concepts font
//@function r_draw_find_system_fonts

//! <TODO: Explain slots and lazy loading>
//! @concepts font
//@function r_draw_font_register

//! <TODO: Explain fallback order>
//! @concepts font
//@function r_draw_font_set_fallbacks

//! <TODO: Explain font instance lookup>
//! @concepts font
//@function r_draw_get_font

//! Returns the line height of the font in unscaled pixels.
//! @see_also r_draw_get_font r_draw_measure_text
//! @concepts font
//@function r_draw_font_line_height

//! <TODO: Explain line breaking, force>
//! @concepts font
//@function r_draw_break_text
