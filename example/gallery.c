#include <SDL3/SDL.h>
#include "rcut_sdl.h"
#include <stdio.h>
#include <string.h>


static struct
{
	bool  notifications, autosave, dark_mode, compact;
	bool  wifi, bluetooth;
	int   quality;
	float volume, brightness, opacity, spacing;
	float count;
	char  name[64], search[128];
	int   language, tab, selected_file;
	bool  section_open, advanced_open;
	int   clicks;
	bool  show_grid, show_rulers;
	char  status[128];
	int   theme, loaded_theme;
	float background;
	r_draw_theme_options look, loaded_look;
	bool  custom_text;
	bool  pill_tabs, loaded_pill_tabs;
	uint32_t text_color;
	float split;
	char  notes[1024];
	bool  tree_open[16];
	int   tree_selected;
} app = {
	.notifications = true, .autosave = false, .wifi = true, .quality = 1,
	.volume = 65, .brightness = 0.4f, .opacity = 80, .spacing = 12, .count = 3,
	.name = "", .section_open = true, .advanced_open = true, .show_grid = true,
	.status = "Ready",
	.split = 0.62f,
	.text_color = 0xFFD4D4D5,
	.notes = "Meeting notes\n- Ship the gallery\n- Review the theme\n- Profile on the Pi Zero",
	.tree_open = { [0] = true, [1] = true },
	.tree_selected = 2,
};

static struct { int depth; char* name; int icon; } tree[] = {
	{ 0, "Projects", R_ICON_FOLDER }, { 1, "rcut", R_ICON_FOLDER }, { 2, "rcut.h", R_ICON_FILE }, { 2, "rcut_core.c", R_ICON_FILE },
	{ 2, "rcut_widgets.c", R_ICON_FILE }, { 1, "website", R_ICON_FOLDER }, { 2, "index.html", R_ICON_FILE }, { 0, "Photos", R_ICON_FOLDER },
	{ 1, "holiday.jpg", R_ICON_FILE }, { 1, "family.png", R_ICON_FILE }, { 0, "Music", R_ICON_FOLDER }, { 1, "album.flac", R_ICON_FILE },
	{ 0, "todo.txt", R_ICON_FILE },
};

static char* languages[] = { "English", "Deutsch", "Français", "Español", "日本語", "Українська" };
static char* tabs[] = { "Files", "Recent" };
static char* themes[] = { "Dark", "Light" };
static const uint32_t theme_backgrounds[] = { 0xFF222226, 0xFFF6F6F6 };
static char* files[] = { "notes.md", "budget-2026.xlsx", "holiday.jpg", "presentation.key", "rcut.c", "rcut_sdl.c", "README", "theme.c", "Makefile", "todo.txt", "screenshot.png", "archive.zip", "music.flac", "report.pdf" };

static r_sdl_frontend* frontend;
static uint64_t status_id;
static bool dirty = true;
static int built_w, built_h;

static void set_status(const char* s) { snprintf(app.status, sizeof(app.status), "%s", s); r_damage_id(&frontend->context, status_id); }
static void on_new(void)   { set_status("New file"); }
static void on_open(void)  { set_status("Open..."); }
static void on_save(void)  { set_status("Saved"); }
static void on_quit(void)  { SDL_Event e = { .type = SDL_EVENT_QUIT }; SDL_PushEvent(&e); }
static void on_undo(void)  { set_status("Undo"); }
static void on_redo(void)  { set_status("Redo"); }
static void on_copy(void)  { set_status("Copied"); }
static void on_paste(void) { set_status("Pasted"); }
static void on_about(void) { set_status("rcut widget gallery"); }
static void on_click_primary(void) { char s[64]; snprintf(s, sizeof(s), "Clicked %d times", ++app.clicks); set_status(s); }
static void on_rename(void) { set_status("Rename"); }
static void on_delete(void) { set_status("Delete"); }

static const r_menu_item recent_menu[] = {
	{ .label = "notes.md", .on_click = on_open },
	{ .label = "budget-2026.xlsx", .on_click = on_open },
	{ 0 },
};
static const r_menu_item file_menu[] = {
	{ .label = "New",       .icon = R_ICON_FILE,   .shortcut = "Ctrl+N", .on_click = on_new },
	{ .label = "Open...",   .icon = R_ICON_FOLDER, .shortcut = "Ctrl+O", .on_click = on_open },
	{ .label = "Open Recent", .submenu = recent_menu },
	{ .label = "Save",      .icon = R_ICON_SAVE,   .shortcut = "Ctrl+S", .on_click = on_save },
	{ .label = "-" },
	{ .label = "Quit",      .icon = R_ICON_CLOSE,  .shortcut = "Ctrl+Q", .on_click = on_quit },
	{ 0 },
};
static const r_menu_item edit_menu[] = {
	{ .label = "Undo",  .icon = R_ICON_UNDO,  .shortcut = "Ctrl+Z", .on_click = on_undo },
	{ .label = "Redo",  .icon = R_ICON_REDO,  .shortcut = "Ctrl+Y", .on_click = on_redo },
	{ .label = "-" },
	{ .label = "Cut",   .icon = R_ICON_CUT,   .shortcut = "Ctrl+X", .disabled = true },
	{ .label = "Copy",  .icon = R_ICON_COPY,  .shortcut = "Ctrl+C", .on_click = on_copy },
	{ .label = "Paste", .icon = R_ICON_PASTE, .shortcut = "Ctrl+V", .on_click = on_paste },
	{ 0 },
};
static const r_menu_item view_menu[] = {
	{ .label = "Show Grid",   .checked = &app.show_grid },
	{ .label = "Show Rulers", .checked = &app.show_rulers },
	{ .label = "-" },
	{ .label = "Zoom In",  .icon = R_ICON_ZOOM_IN,  .shortcut = "Ctrl++" },
	{ .label = "Zoom Out", .icon = R_ICON_ZOOM_OUT, .shortcut = "Ctrl+-" },
	{ 0 },
};
static const r_menu_item help_menu[] = {
	{ .label = "About", .icon = R_ICON_INFO, .on_click = on_about },
	{ 0 },
};
static const r_menu_item menubar[] = {
	{ .label = "File", .submenu = file_menu },
	{ .label = "Edit", .submenu = edit_menu },
	{ .label = "View", .submenu = view_menu },
	{ .label = "Help", .submenu = help_menu },
	{ 0 },
};

#define DIALOG_ID 0xD1A106
#define CARD    (R_STYLE_PANEL | R_VARIANT_RAISED)
#define CAPTION (R_STYLE_LABEL | R_VARIANT_DULL)
// Looks of the rich text, added to the theme by rich_text_styles.
#define RICH_BOLD     (R_STYLE_LABEL | R_VARIANT_BOLD)
#define RICH_TITLE    (R_STYLE_HEADER | R_VARIANT_RAISED)
#define RICH_SHADOWED (R_STYLE_LABEL | R_VARIANT_LISTITEM)

static r_ui_list* dialog;
static char* dialog_text = "Your display name is shown to other people in shared documents, comments and notifications.";

static void open_dialog(void)
{
	if(dialog) return;
	int text_h = r_measure_wrapped(&frontend->context, CAPTION, dialog_text, 380 - 40).h;
	dialog = r_sdl_dialog(frontend, DIALOG_ID, 380, 150 + text_h, "Edit profile");
	dirty = true;
}

static void close_dialog(void)
{
	r_destroy_ui_list(dialog);
	dialog = NULL;
}
static void on_submit(char* text) { char s[128]; snprintf(s, sizeof(s), "Hello, %s!", text); set_status(s); }

static void toggle_advanced(void) { app.advanced_open = !app.advanced_open; dirty = true; }
static bool tree_is_open(int i) { return app.tree_open[i]; }
static void tree_set_open(int i, bool open) { app.tree_open[i] = open; dirty = true; }


static r_layout_iter card_begin(r_layout_iter* column, char* title)
{
	r_layout_iter card = r_group_begin(column, CARD);
	r_label_ex(column->l, r_next(&card, 18), R_STYLE_LABEL | R_VARIANT_BOLD, R_ICON_NONE, title);
	return card;
}

static r_layout_iter column_begin(r_ui_list* l, rect r, r_ui_list** page)
{
	*page = r_scroll_begin_ex(l, r_grow(r, 6), 0);
	return r_stack(*page, (rect){ 6, 6, (*page)->root_area.w - 12, 100000, r.id }, R_TOP);
}

static void column_end(r_ui_list* l, r_ui_list* page, r_layout_iter* column)
{
	page->document_area.h = column->start.h - column->area.h + 12;
	r_scroll_end(l, page);
}

static void pill_tabs(r_draw* c)
{
	const r_draw_palette* p = &c->palette;
	r_theme(c, R_STYLE_TAB_BAR); r_theme_make_box(c);
	r_theme(c, R_STYLE_TAB | R_STATE_HOVER);    r_theme_label_color(c, p->text);
	r_theme(c, R_STYLE_TAB | R_STATE_SELECTED); r_theme_fill(c, p->selection_subtle, 0); r_theme_label_color(c, p->text_strong);
	r_theme(c, R_STYLE_TAB); r_theme_radius(c, 13); r_theme_padding(c, 12, 0); r_theme_label(c, p->text_dim, R_CENTER); r_theme_focus(c, R_FOCUS_RING);
	r_theme_make_box(c); r_theme_apply_states(c, R_STATE_HOVER | R_STATE_SELECTED);
}

static void rich_text_styles(r_draw* c)
{
	const r_draw_palette* p = &c->palette;
	int size = (int)(app.look.font_size + 0.5f);
	r_theme(c, RICH_TITLE);
	r_theme_font(c, r_draw_get_font(c, 0, size * 3 / 2, R_FONT_BOLD | R_FONT_SHAPED));
	r_theme_label_color(c, 0xFFFFFFFF);
	r_theme_text_outline(c, 2, p->primary);
	r_theme_text_shadow(c, 0, 2, 2, r_rgba_f(0, 0, 0, 0.5f));
	r_theme_make_box(c);
	r_theme(c, RICH_SHADOWED);
	r_theme_font(c, r_draw_get_font(c, 0, size, R_FONT_SEMIBOLD | R_FONT_SHAPED));
	r_theme_label_color(c, p->primary);
	r_theme_text_shadow(c, 1, 1, 1, r_rgba_f(0, 0, 0, p->dark ? 0.8f : 0.3f));
	r_theme_make_box(c);
}

static void update_theme(r_sdl_frontend* f)
{
	if(app.theme != app.loaded_theme) app.background = app.theme * 100.0f;
	app.theme = app.loaded_theme = app.background >= 50;
	app.look.background = 0xFF000000;
	for(int i = 0; i < 24; i += 8)
	{
		float a = (theme_backgrounds[0] >> i) & 0xFF, b = (theme_backgrounds[1] >> i) & 0xFF;
		app.look.background |= (uint32_t)(a + (b - a) * app.background / 100 + 0.5f) << i;
	}
	app.look.text = app.custom_text ? app.text_color | 0xFF000000 : 0;
	if(memcmp(&app.look, &app.loaded_look, sizeof(app.look)) == 0 && app.pill_tabs == app.loaded_pill_tabs) return;
	r_theme_default(&f->draw, &app.look);
	if(app.pill_tabs) pill_tabs(&f->draw);
	rich_text_styles(&f->draw);
	memcpy(&app.loaded_look, &app.look, sizeof(app.look));
	app.loaded_pill_tabs = app.pill_tabs;
	dirty = true;
}

static void build(r_sdl_frontend* f)
{
	r_ui_list* l = f->root;
	r_context* c = &f->context;
	r_list_clear(l);

	r_layout_iter window = r_stack(l, l->root_area, R_TOP);
	window.spacing = 0;
	r_menubar(l, r_next(&window, 34), menubar);
	r_layout_iter footer = r_stack(l, r_remaining(&window), R_BOTTOM);
	footer.spacing = 0;
	rect status = r_next(&footer, 28);
	status_id = status.id;
	r_panel(l, status, R_STYLE_MENUBAR_BG);
	r_label_ex(l, r_shrink_sides(status, 12, 0, 12, 0), CAPTION, R_ICON_INFO, app.status);
	r_layout_iter columns = r_stack(l, r_shrink_sides(r_remaining(&footer), 16, 12, 16, 12), R_LEFT);
	columns.spacing = 16;
	rect col_rects[3] = { r_next_fraction(&columns, 0.32f), r_next_fraction(&columns, 0.34f), r_remaining(&columns) };
	r_ui_list* page[3];

	r_layout_iter col = column_begin(l, col_rects[0], &page[0]);
	r_ui_list* p = page[0];

	r_layout_iter card = card_begin(&col, "Buttons");
	r_layout_iter row = r_row(&card, -1, R_LEFT);
	rect primary = r_next_fraction(&row, 0.5f);
	r_button_ex(p, primary, R_STYLE_BUTTON | R_VARIANT_BOLD, R_ICON_NONE, "Primary");
	r_on_click(p, primary, on_click_primary);
	r_tooltip(p, primary, "Increments the click counter. Long tooltips are word wrapped to keep them readable.");
	rect secondary = r_remaining(&row);
	r_button(p, secondary, "Secondary");
	r_on_click(p, secondary, on_click_primary);
	row = r_row(&card, -1, R_LEFT);
	rect flat = r_next_fraction(&row, 0.5f);
	r_button_ex(p, flat, R_STYLE_BUTTON | R_VARIANT_DULL, R_ICON_SETTINGS, "Flat");
	r_on_click(p, flat, on_click_primary);
	r_button_ex(p, r_remaining(&row), R_STYLE_BUTTON | R_STATE_DISABLED, R_ICON_NONE, "Disabled");
	row = r_row(&card, -1, R_LEFT);
	int icons[] = { R_ICON_HOME, R_ICON_SEARCH, R_ICON_STAR, R_ICON_HEART, R_ICON_BELL, R_ICON_TRASH };
	for(int i = 0; i < 6; i++)
	{
		rect icon_button = r_next(&row, -1);
		r_button_ex(p, icon_button, R_STYLE_BUTTON, icons[i], NULL);
		r_on_click(p, icon_button, on_click_primary);
	}
	r_group_end(&card, &col);

	card = card_begin(&col, "Preferences");
	r_checkbox(p, r_next(&card, 22), "Enable notifications", R_PTR(&app.notifications));
	r_checkbox(p, r_next(&card, 22), "Autosave documents", R_PTR(&app.autosave));
	r_toggle(p, r_next(&card, 22), "Wi-Fi", R_PTR(&app.wifi));
	r_toggle(p, r_next(&card, 22), "Bluetooth", R_PTR(&app.bluetooth));
	r_separator(p, r_next(&card, 1));
	row = r_row(&card, 22, R_LEFT);
	char* qualities[] = { "Low", "Medium", "High" };
	for(int i = 0; i < 3; i++)
		r_radio(p, i < 2 ? r_next_fraction(&row, 0.33f) : r_remaining(&row), qualities[i], i, R_PTR(&app.quality));
	r_group_end(&card, &col);

	card = card_begin(&col, "Advanced");
	rect section = r_next(&card, -1);
	int open = app.advanced_open ? R_STATE_TOGGLED : 0;
	r_button_ex(p, section, R_STYLE_COLLAPSIBLE | open, open ? R_ICON_HIDE_DETAILS : R_ICON_SHOW_DETAILS, "Experimental features");
	r_on_click(p, section, toggle_advanced);
	if(app.advanced_open)
	{
		r_checkbox(p, r_next(&card, 22), "Compact layout", R_PTR(&app.compact));
		r_checkbox(p, r_next(&card, 22), "Developer mode", R_PTR(&app.dark_mode));
	}
	else r_label_ex(p, r_next(&card, 22), CAPTION, R_ICON_NONE, "Two hidden options");
	r_group_end(&card, &col);

	card = card_begin(&col, "Theme");
	row = r_row(&card, -1, R_LEFT);
	r_label_ex(p, r_next(&row, 88), CAPTION, R_ICON_NONE, "Preset");
	r_segmented(p, r_remaining(&row), themes, 2, R_PTR(&app.theme));
	r_slider_box(p, r_next(&card, 26), "Brightness", 0, 100, 1, R_PTR(&app.background));
	struct { char* name; uint32_t* color; } colors[] = { { "Label", &app.text_color }, { "Primary", &app.look.primary } };
	for(int i = 0; i < (int)(sizeof(colors) / sizeof(colors[0])); i++)
	{
		if(i == 0) r_checkbox(p, r_next(&card, 22), "Custom label color", R_PTR(&app.custom_text));
		if(i == 0 && !app.custom_text) continue;
		row = r_row(&card, 26, R_LEFT);
		r_label_ex(p, r_next(&row, 88), CAPTION, R_ICON_NONE, colors[i].name);
		int w = (row.area.w - 3 * row.spacing) / 3;
		r_slider_box(p, r_next(&row, w), "R", 0, 255, 1, R_BITFIELD(colors[i].color, 16, 8));
		r_slider_box(p, r_next(&row, w), "G", 0, 255, 1, R_BITFIELD(colors[i].color, 8, 8));
		r_slider_box(p, r_remaining(&row), "B", 0, 255, 1, R_BITFIELD(colors[i].color, 0, 8));
	}
	r_slider_box(p, r_next(&card, 26), "Font size", 9, 24, 1, R_PTR(&app.look.font_size));
	r_checkbox(p, r_next(&card, 22), "Pill tabs", R_PTR(&app.pill_tabs));
	r_checkbox(p, r_next(&card, 22), "Scrollbar buttons", R_PTR(&app.look.scrollbar_buttons));
	r_group_end(&card, &col);
	column_end(l, page[0], &col);

	col = column_begin(l, col_rects[1], &page[1]);
	p = page[1];

	card = card_begin(&col, "Profile");
	r_label_ex(p, r_next(&card, 16), CAPTION, R_ICON_NONE, "Name");
	rect name = r_next(&card, -1);
	r_line_edit(p, name, app.name, sizeof(app.name), "Your name");
	r_line_edit_on_submit(p, name, on_submit);
	r_label_ex(p, r_next(&card, 16), CAPTION, R_ICON_NONE, "Search");
	r_line_edit(p, r_next(&card, -1), app.search, sizeof(app.search), "Search files...");
	row = r_row(&card, -1, R_LEFT);
	r_dropdown(p, r_next_fraction(&row, 0.6f), languages, 6, R_PTR(&app.language));
	r_spinbox(p, r_remaining(&row), 0, 99, 1, R_PTR(&app.count));
	row = r_row(&card, -1, R_LEFT);
	rect edit = r_next_fraction(&row, 0.5f), save = r_remaining(&row);
	r_button_ex(p, edit, R_STYLE_BUTTON, R_ICON_EDIT, "Edit...");
	r_on_click(p, edit, open_dialog);
	r_button_ex(p, save, R_STYLE_BUTTON | R_VARIANT_BOLD, R_ICON_SAVE, "Save");
	r_on_click(p, save, on_save);
	r_group_end(&card, &col);

	card = card_begin(&col, "Display");
	row = r_row(&card, 22, R_LEFT);
	r_label_ex(p, r_next(&row, 88), CAPTION, R_ICON_NONE, "Volume");
	r_slider(p, r_remaining(&row), 0, 100, 1, R_PTR(&app.volume));
	row = r_row(&card, 22, R_LEFT);
	r_label_ex(p, r_next(&row, 88), CAPTION, R_ICON_NONE, "Brightness");
	r_slider(p, r_remaining(&row), 0, 1, 0, R_PTR(&app.brightness));
	r_slider_box(p, r_next(&card, 26), "Opacity", 0, 100, 1, R_PTR(&app.opacity));
	r_slider_box(p, r_next(&card, 26), "Spacing", 0, 32, 0.5f, R_PTR(&app.spacing));
	r_group_end(&card, &col);

	card = card_begin(&col, "Storage");
	struct { char* name; float used; } disks[] = { { "Documents", 0.72f }, { "Photos", 0.35f }, { "Cache", 0.18f } };
	static char percent[3][16];
	for(int i = 0; i < 3; i++)
	{
		snprintf(percent[i], sizeof(percent[i]), "%d%%", (int)(disks[i].used * 100));
		r_layout_iter disk = r_group_begin(&card, 0);
		disk.spacing = 2;
		row = r_row(&disk, 20, R_RIGHT);
		r_label_ex(p, r_next(&row, 40), CAPTION, R_ICON_NONE, percent[i]);
		r_label(p, r_remaining(&row), disks[i].name);
		r_progress(p, r_next(&disk, 6), disks[i].used);
		r_group_end(&disk, &card);
	}
	r_group_end(&card, &col);

	card = card_begin(&col, "Rich text");
	r_layout_iter text = r_flow_begin(&card, R_STYLE_LABEL);
	r_text(&text, RICH_TITLE, "Outlined Title\n");
	r_text(&text, 0, "Text flows through ");
	r_text(&text, RICH_BOLD, "bold");
	r_text(&text, 0, ", ");
	r_text(&text, CAPTION, "dim");
	r_text(&text, 0, " and ");
	r_text(&text, RICH_SHADOWED, "shadowed");
	r_text(&text, 0, " styles, around widgets like ");
	rect inline_button = r_next_fit(&text, R_STYLE_BUTTON, "this one");
	r_button(p, inline_button, "this one");
	r_on_click(p, inline_button, on_click_primary);
	r_text(&text, 0, " and icons ");
	r_icon(p, r_next(&text, -1), 0, R_ICON_HEART);
	r_text(&text, 0, ". Shaped fonts kern (AVATAR, Type) and join ligatures (office, fjord), emoji too: \xF0\x9F\x98\x80 \xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD.");
	r_flow_end(&text, &card);
	r_group_end(&card, &col);
	column_end(l, page[1], &col);

	col = column_begin(l, col_rects[2], &page[2]);
	p = page[2];
	rect library = col_rects[2];
	library = (rect){ 0, 0, library.w, SDL_max(page[2]->root_area.h - 12, 420), library.id };
	library = r_next(&col, library.h);
	r_panel(p, library, CARD);
	r_layout_iter lib = r_stack(p, r_inner(c, CARD, library), R_TOP);
	r_label_ex(p, r_next(&lib, 18), R_STYLE_LABEL | R_VARIANT_BOLD, R_ICON_NONE, "Library");
	rect top, bottom;
	r_split(p, r_remaining(&lib), R_TOP, R_PTR(&app.split), 80, 80, &top, &bottom);
	r_ui_list* pages[2];
	r_tabs(p, top, tabs, 2, R_PTR(&app.tab), pages);
	for(int tab = 0; tab < 2; tab++)
	{
		r_ui_list* v = r_scroll_begin_ex(pages[tab], pages[tab]->document_area, 0);
		r_layout_iter list = r_stack(v, (rect){ 4, 0, v->document_area.w - 8, 100000, v->document_area.id }, R_TOP);
		list.spacing = 2;
		if(tab == 0)
		{
			int n = sizeof(tree) / sizeof(tree[0]), hide_below = 1000;
			for(int i = 0; i < n; i++)
			{
				if(tree[i].depth > hide_below) continue;
				hide_below = 1000;
				bool has_children = i + 1 < n && tree[i + 1].depth > tree[i].depth;
				rect item = r_next_keyed(&list, 26, i);
				r_data_binding open = R_GETSET(.getter_b_i32 = tree_is_open, .setter_i32_b = tree_set_open, .index = i);
				if(!r_tree_node(v, item, tree[i].depth, tree[i].name, tree[i].icon, has_children, open, i, R_PTR(&app.tree_selected)) && has_children)
					hide_below = tree[i].depth;
				r_on_context(v, item, "Rename", on_rename);
				r_on_context(v, item, "Delete", on_delete);
			}
		}
		else
		{
			for(int i = 0; i < 14; i++)
			{
				rect item = r_next_keyed(&list, 26, i);
				r_selectable(v, item, files[i], i, R_PTR(&app.selected_file));
				r_on_context(v, item, "Rename", on_rename);
				r_on_context(v, item, "Delete", on_delete);
			}
		}
		r_scroll_end(pages[tab], v);
	}
	r_layout_iter notes = r_stack(p, bottom, R_TOP);
	r_label_ex(p, r_next(&notes, 18), CAPTION, R_ICON_EDIT, "Notes");
	r_text_area(p, r_remaining(&notes), app.notes, sizeof(app.notes), "Write something...");
	column_end(l, page[2], &col);

}

static void build_dialog(r_sdl_frontend* f)
{
	r_ui_list* d = dialog;
	rect area = r_dialog_begin(d);
	int text_h = r_measure_wrapped(&f->context, CAPTION, dialog_text, area.w).h;
	r_layout_iter form = r_stack(d, area, R_TOP);
	r_paragraph(d, r_next(&form, text_h), CAPTION, dialog_text);
	r_label_ex(d, r_next(&form, 18), CAPTION, R_ICON_NONE, "Display name");
	r_line_edit(d, r_next(&form, -1), app.name, sizeof(app.name), "Your name");
	r_layout_iter bottom_up = r_stack(d, r_remaining(&form), R_BOTTOM);
	r_layout_iter buttons = r_row(&bottom_up, -1, R_RIGHT);
	rect ok = r_next(&buttons, 90), cancel = r_next(&buttons, 90);
	r_button_ex(d, ok, R_STYLE_BUTTON | R_VARIANT_BOLD, R_ICON_NONE, "OK");
	r_on_click(d, ok, close_dialog);
	r_button(d, cancel, "Cancel");
	r_on_click(d, cancel, close_dialog);
}

static void rebuild(r_sdl_frontend* f)
{
	if(!dirty && f->root->root_area.w == built_w && f->root->root_area.h == built_h) return;
	dirty = false; built_w = f->root->root_area.w; built_h = f->root->root_area.h;
	build(f);
	if(dialog) build_dialog(f);
}

static void frame(r_sdl_frontend* f)
{
	if(dialog && r_sdl_close_requested(f, dialog, true)) close_dialog();
	update_theme(f);
	rebuild(f);
	r_sdl_update(f);
	rebuild(f);
}

int main(int argc, char** argv)
{
	r_sdl_frontend* f = r_sdl_init("rcut gallery", 1040, 720, R_SDL_RESIZABLE | R_SDL_VSYNC);
	if(!f) return 1;
	frontend = f;
	app.look = app.loaded_look = r_draw_default_theme_options;
	rich_text_styles(&f->draw);
	build(f);
	while(!r_sdl_quit_requested(f, true))
	{
		SDL_Event e;
		if(SDL_WaitEventTimeout(&e, 50))
		{
			do r_sdl_process_event(f, &e); while(SDL_PollEvent(&e));
		}
		frame(f);
		r_sdl_render_ui(f);
	}
	r_sdl_free(f);
	return 0;
}
