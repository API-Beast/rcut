#include "internal.h"

static rect window_rect(r_ui_list* l)
{
	int ox, oy; rect clip;
	r_list_get_transform(l, &ox, &oy, &clip);
	r_ui_list* parent = l;
	while(parent->ephemeral_parent)
		parent = parent->ephemeral_parent;
	return r_offset(parent->root_area, -ox, -oy);
}

rect r_popup_place(r_ui_list* parent, rect anchor, int w, int h, r_alignment side)
{
	rect win = window_rect(parent);
	rect r = {.w = w, .h = h, .id = anchor.id};
	if(side & R_RIGHT)
	{
		r.x = anchor.x + anchor.w; r.y = anchor.y;
		if(r.x + w > win.x + win.w && anchor.x - w >= win.x) r.x = anchor.x - w;
	}
	else
	{
		r.x = anchor.x; r.y = anchor.y + anchor.h;
		if(r.y + h > win.y + win.h && anchor.y - h >= win.y) r.y = anchor.y - h;
	}
	r.x = MAX(win.x, MIN(r.x, win.x + win.w - w));
	r.y = MAX(win.y, MIN(r.y, win.y + win.h - h));
	return r;
}

static void close_popup(r_ui_list* p)
{
	if(p && p->ephemeral_parent) r_ephemeral_mark_to_delete(p->ephemeral_parent, p->root_area.id);
}

static bool popup_is_open(r_context* c, rect r, uint64_t salt)
{
	r_ui_list* p = hash_getv(&c->ephemeral_id_map, r_salt(r, salt).id);
	return p && !p->is_marked_as_to_delete;
}

static bool has_activation(r_interact* it)
{
	for(r_callback* cb = it->callbacks; cb; cb = cb->next)
	if(cb->event == R_ACTIVATION && cb->function)
		return true;

	return false;
}

static bool popup_focus(r_context* c, r_ui_list* popup, int dir, bool relative)
{
	int n = popup->count_interact_items, cur = -1;
	if(n == 0) return false;

	for(int i = 0; i < n && relative; i++)
	if(popup->interact_items[i].r.id == c->focus_id)
		cur = i;

	int i = cur < 0 ? (dir > 0 ? 0 : n - 1) : cur + dir;
	for(int k = 0; k < n; k++, i += dir)
	{
		i = (i + n) % n;
		if(i > 0 && popup->interact_items[i - 1].r.id == popup->interact_items[i].r.id) continue;
		if(!has_activation(&popup->interact_items[i])) continue;
		c->is_in_navigation_mode = true;
		r_change_focus(c, &popup->interact_items[i]);
		return true;
	}
	return false;
}

static bool tooltip_mouse_hover(const r_event* e, r_callback* self)
{
	if(e->context->pressed_id) return false;
	r_context* c = e->context;
	r_ui_list* p = r_popup(e->ui_list, self->parent->r, R_POPUP_TOOLTIP, P400);
	if(p->count_draw_items == 0)
	{
		char* text = r_store(p, self->data, self->size);
		rect inner = r_measure_wrapped(c, R_STYLE_TOOLTIP, text, 320);
		rect outer = r_outer(c, R_STYLE_TOOLTIP, &inner);
		rect cursor = {.x = (int)e->list_mouse_x, .y = (int)e->list_mouse_y, .w = 14, .h = 22}; // Roughly the size of an arrow pointer.
		outer = r_popup_place(e->ui_list, cursor, outer.w, outer.h, R_RIGHT);
		outer.id = p->root_area.id;
		p->root_area = outer;
		r_drawable* d = r_style(p, outer, R_STYLE_TOOLTIP);
		d->text = text;
		d->wrap_label = true;
	}
	r_list_set_visibility(p, true);
	return true;
}

static bool tooltip_close(const r_event* e, r_callback* self)
{
	r_popup_mark_to_delete(e->ui_list, self->parent->r, P400);
	return false;
}

void r_tooltip(r_ui_list* l, rect r, const char* text)
{
	r_popup_keep_alive(l, r, P400);
	r_callback* hover = r_attach_callback(l, r, R_MOUSE_HOVER,   tooltip_mouse_hover);
	r_callback* out   = r_attach_callback(l, r, R_MOUSE_OUT,     tooltip_close);
	r_callback* press = r_attach_callback(l, r, R_MOUSE_PRESSED, tooltip_close);
	const char* end = memchr(text, 0, 512);
	int size = end ? (int)(end - text) + 1 : 512;
	r_callback_set_data  (l, hover, text, size);
	((char*)hover->data)[size - 1] = 0;
	r_callback_share_data(l, out, hover);
	r_callback_share_data(l, press, hover);
}

struct menu_item_data
{
	const r_menu_item* item;
	r_ui_list* menu;
	rect       item_rect;
	uint64_t   context_target;
	int        context_index;
};

static void close_submenus(r_ui_list* menu, uint64_t except_anchor)
{
	r_context* c = menu->context;
	for(int i = 0; i < c->count_ephemeral; i++)
	{
		r_ui_list* e = &c->ephemeral[i];
		if(e->context && e->ephemeral_parent == menu && !e->is_viewport && e->anchor_id != except_anchor && !e->is_marked_as_to_delete)
			close_popup(e);
	}
}

static bool menu_item_activate(const r_event* e, r_callback* self)
{
	struct menu_item_data* data = self->data;
	const r_menu_item* item = data->item;
	r_context* c = e->context;
	if(item->submenu)
	{
		r_ui_list* sub = r_menu_popup(data->menu, data->item_rect, item->submenu, R_RIGHT);
		if(c->is_in_navigation_mode && sub->count_interact_items) r_change_focus(c, &sub->interact_items[0]);
		return true;
	}
	if(item->checked) *item->checked = !*item->checked;
	if(item->on_click) item->on_click();
	if(data->context_target)
	{
		r_interact* target = r_find_interact(c, data->context_target);
		int n = 0;
		if(target)
		for(r_interact* part = target; part->r.id == target->r.id; part++)
		for(r_callback* cb = part->callbacks; cb; cb = cb->next)
		{
			if(cb->event != R_CONTEXT || n++ != data->context_index) continue;
			r_event ce = *e;
			ce.ui_list = target->owner;
			if(cb->function) cb->function(&ce, cb);
		}
	}
	r_close_popups(c, false);
	return true;
}

static bool menu_item_hover(const r_event* e, r_callback* self)
{
	struct menu_item_data* data = self->data;
	close_submenus(data->menu, data->item_rect.id);
	if(data->item->submenu && !data->item->disabled)
		r_menu_popup(data->menu, data->item_rect, data->item->submenu, R_RIGHT);
	return true;
}

static bool menu_item_key(const r_event* e, r_callback* self)
{
	struct menu_item_data* data = self->data;
	r_context* c = e->context;
	r_ui_list* menu = data->menu;
	switch(e->keycode)
	{
		case R_KEY_UP:   return popup_focus(c, menu, -1, true);
		case R_KEY_DOWN: return popup_focus(c, menu,  1, true);
		case R_KEY_RIGHT:
			if(!data->item->submenu || data->item->disabled) return false;
			return popup_focus(c, r_menu_popup(menu, data->item_rect, data->item->submenu, R_RIGHT), 1, false);
		case R_KEY_LEFT:
		{
			r_ui_list* parent = menu->ephemeral_parent;
			if(!parent->close_on_outside_click) return false;
			r_interact* anchor = r_list_find_this_frame_interact(parent, menu->anchor_id);
			close_popup(menu);
			if(anchor) r_change_focus(c, anchor);
			return true;
		}
	}
	return false;
}

static void menu_build(r_ui_list* p, r_ui_list* parent, rect anchor, const r_menu_item* items, r_alignment side, uint64_t context_target)
{
	r_context* c = p->context;
	int icon = r_layout_default_size(c, R_STYLE_ICON);
	int item_h = r_layout_default_size(c, R_STYLE_MENU_ITEM);
	int gap    = r_layout_default_spacing(c, R_STYLE_MENU_ITEM);
	int sep_h  = gap + 1;

	int label_w = 0, shortcut_w = 0, h = 0;
	bool any_icon = false, any_arrow = false;
	for(int i = 0; items[i].label; i++)
	{
		if(strcmp(items[i].label, "-") == 0) { h += sep_h; continue; }
		label_w = MAX(label_w, r_measure(c, R_STYLE_MENU_ITEM, items[i].label).w);
		if(items[i].shortcut) shortcut_w = MAX(shortcut_w, r_measure(c, R_STYLE_MENU_ITEM | R_VARIANT_TERTIARY, items[i].shortcut).w);
		any_icon  |= items[i].icon || items[i].checked;
		any_arrow |= items[i].submenu != NULL;
		h += item_h;
	}
	int content_w = label_w + (any_icon ? icon + gap : 0) + (shortcut_w ? shortcut_w + gap * 3 : 0) + (any_arrow ? icon + gap : 0);
	rect item_inner = {.w = content_w, .h = item_h};
	rect item_outer = r_outer(c, R_STYLE_MENU_ITEM, &item_inner);
	rect list_inner = {.w = item_outer.w, .h = h};
	rect bg = r_outer(c, R_STYLE_MENU_BG, &list_inner);
	if((side & R_RIGHT) && parent->ephemeral_parent && !parent->is_viewport)
	{
		rect inner = r_inner(c, R_STYLE_MENU_BG, bg);
		anchor.x = parent->root_area.x;
		anchor.w = parent->root_area.w;
		anchor.y -= inner.y - bg.y;
	}
	bg = r_popup_place(parent, anchor, MAX(bg.w, side & R_RIGHT ? 0 : anchor.w), bg.h, side);
	bg.id = p->root_area.id;
	p->root_area = bg;

	r_style(p, bg, R_STYLE_MENU_BG);
	rect area = r_inner(c, R_STYLE_MENU_BG, bg);
	int ctx_index = 0;
	for(int i = 0; items[i].label; i++)
	{
		const r_menu_item* item = &items[i];
		if(strcmp(item->label, "-") == 0)
		{
			r_separator(p, r_cut_px(R_TOP, &area, sep_h, i));
			continue;
		}
		rect ir = r_cut_px(R_TOP, &area, item_h, i);
		int state = item->disabled ? R_STATE_DISABLED : 0;
		r_style(p, ir, R_STYLE_MENU_ITEM | state);
		rect inner = r_inner(c, R_STYLE_MENU_ITEM, ir);
		if(any_icon)
		{
			rect ic = r_cut_px(R_LEFT, &inner, icon + gap, P450);
			int glyph = item->checked ? (*item->checked ? R_ICON_CHECKED : R_ICON_NONE) : item->icon;
			if(glyph) { r_drawable* d = r_style(p, ic, R_STYLE_ICON); d->icon = glyph; d->outer_style = R_STYLE_MENU_ITEM | state; d->parent_id = ir.id; }
		}
		if(any_arrow)
		{
			rect ar = r_cut_px(R_RIGHT, &inner, icon, P470);
			if(item->submenu) { r_drawable* d = r_style(p, ar, R_STYLE_ICON); d->icon = R_ICON_RIGHT; d->outer_style = R_STYLE_MENU_ITEM | state; d->parent_id = ir.id; }
		}
		if(item->shortcut)
		{
			int sw = r_measure(c, R_STYLE_MENU_ITEM | R_VARIANT_TERTIARY, item->shortcut).w;
			rect sr = r_cut_px(R_RIGHT, &inner, sw, P460);
			r_label_ex(p, sr, R_STYLE_MENU_ITEM | R_VARIANT_TERTIARY | state, R_ICON_NONE, item->shortcut)->parent_id = ir.id;
		}
		r_label_ex(p, inner, R_STYLE_MENU_ITEM | state, R_ICON_NONE, item->label)->parent_id = ir.id;

		struct menu_item_data data = { item, p, ir, context_target, ctx_index++ };
		r_callback* hover = r_attach_callback(p, ir, R_MOUSE_IN, menu_item_hover);
		r_callback_set_data(p, hover, &data, sizeof(data));
		if(item->disabled) continue;
		r_callback* act = r_attach_callback(p, ir, R_ACTIVATION, menu_item_activate);
		r_callback_share_data(p, act, hover);
		r_callback* key = r_attach_callback(p, ir, R_KEY_INPUT, menu_item_key);
		r_callback_share_data(p, key, hover);
		act->parent->traps_arrow_focus = true;
	}
}

r_ui_list* r_menu_popup(r_ui_list* parent, rect anchor, const r_menu_item* items, r_alignment side)
{
	r_ui_list* p = r_popup(parent, anchor, R_POPUP_AUTOCLOSE, P410);
	if(p->count_draw_items == 0)
		menu_build(p, parent, anchor, items, side, 0);
	r_list_set_visibility(p, true);
	return p;
}

// "Ctrl+Shift+S" for display next to a context menu entry.
static char* hotkey_name(r_ui_list* l, uint32_t key, uint32_t modifiers)
{
	static const struct { uint32_t key; const char* name; } names[] = {
		{ R_KEY_BACKSPACE, "Backspace" }, { R_KEY_TAB, "Tab" }, { R_KEY_ENTER, "Enter" }, { R_KEY_ESCAPE, "Esc" }, { R_KEY_SPACE, "Space" },
		{ R_KEY_PAGEUP, "PgUp" }, { R_KEY_PAGEDOWN, "PgDn" }, { R_KEY_END, "End" }, { R_KEY_HOME, "Home" },
		{ R_KEY_LEFT, "Left" }, { R_KEY_UP, "Up" }, { R_KEY_RIGHT, "Right" }, { R_KEY_DOWN, "Down" }, { R_KEY_DELETE, "Del" },
	};
	char key_buf[2] = { (char)key, 0 };
	const char* key_name = (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9') ? key_buf : "?";
	for(int i = 0; i < COUNTOF(names); i++)
		if(names[i].key == key) key_name = names[i].name;
	char buf[64];
	snprintf(buf, sizeof(buf), "%s%s%s%s%s",
		modifiers & R_MOD_CTRL  ? "Ctrl+"  : "", modifiers & R_MOD_ALT ? "Alt+" : "",
		modifiers & R_MOD_SHIFT ? "Shift+" : "", modifiers & R_MOD_WIN ? "Win+" : "", key_name);
	return r_store_string(l, buf);
}

void __core_open_context_menu(r_interact* it, float list_x, float list_y)
{
	r_ui_list* l = it->owner;
	r_ui_list* p = r_popup(l, it->r, R_POPUP_AUTOCLOSE, P430);
	r_list_clear(p);
	int n = 0;
	for(r_interact* part = it; part->r.id == it->r.id; part++)
	for(r_callback* cb = part->callbacks; cb; cb = cb->next) n += cb->event == R_CONTEXT;
	r_menu_item* items = r_storage_alloc(p, sizeof(r_menu_item) * (n + 1));
	memset(items, 0, sizeof(r_menu_item) * (n + 1));
	n = 0;

	for(r_interact* part = it; part->r.id == it->r.id; part++)
	for(r_callback* cb = part->callbacks; cb; cb = cb->next)
	if(cb->event == R_CONTEXT)
	{
		items[n].label = cb->menu_label ? cb->menu_label : "?";
		if(cb->hotkey) items[n].shortcut = hotkey_name(p, cb->hotkey, cb->hotkey_modifier);
		n++;
	}

	rect anchor = {.x = (int)list_x, .y = (int)list_y, .w = 1, .h = 1};
	menu_build(p, l, anchor, items, R_BOTTOM, it->r.id);
	r_list_set_visibility(p, true);
}

static bool context_fn_wrapper(const r_event* e, r_callback* self)
{
	void (*fn)(void) = *(void(**)(void))self->data;
	if(fn) fn();
	return true;
}

r_callback* r_on_context(r_ui_list* l, rect r, const char* label, void (*on_select)(void))
{
	r_popup_keep_alive(l, r, P430);
	r_callback* cb = r_attach_callback(l, r, R_CONTEXT, context_fn_wrapper);
	r_callback_set_data(l, cb, &on_select, sizeof(on_select));
	cb->menu_label = (char*)label;
	return cb;
}

struct menubar_data
{
	const r_menu_item* menu;
	rect item_rect;
	uint64_t bar_id;
};

static r_ui_list* open_menubar_popup(r_ui_list* l, uint64_t bar_id, uint64_t except)
{
	r_context* c = l->context;
	for(int i = 0; i < c->count_ephemeral; i++)
	{
		r_ui_list* e = &c->ephemeral[i];
		if(!e->context || e->ephemeral_parent != l || e->is_marked_as_to_delete || e->anchor_id == except) continue;
		r_interact* anchor = r_list_find_this_frame_interact(l, e->anchor_id);
		if(anchor && anchor->press_group == bar_id) return e;
	}
	return NULL;
}

static bool menubar_activate(const r_event* e, r_callback* self)
{
	struct menubar_data* data = self->data;
	r_ui_list* open = r_popup_keep_alive(e->ui_list, data->item_rect, P410);
	if(open) close_popup(open);
	else
	{
		r_ui_list* p = r_menu_popup(e->ui_list, data->item_rect, data->menu->submenu, R_BOTTOM);
		if(e->context->is_in_navigation_mode && p->count_interact_items) r_change_focus(e->context, &p->interact_items[0]);
	}
	return true;
}

static bool menubar_key(const r_event* e, r_callback* self)
{
	struct menubar_data* data = self->data;
	r_context* c = e->context;
	if(e->keycode == R_KEY_DOWN)
	{
		r_ui_list* p = r_popup_keep_alive(e->ui_list, data->item_rect, P410);
		if(!p) p = r_menu_popup(e->ui_list, data->item_rect, data->menu->submenu, R_BOTTOM);
		return popup_focus(c, p, 1, false);
	}
	if(e->keycode == R_KEY_LEFT || e->keycode == R_KEY_RIGHT)
	{
		r_interact* items[64]; int n = 0, cur = 0;
		for(int i = 0; i < e->ui_list->count_interact_items && n < 64; i++)
		{
			r_interact* it = &e->ui_list->interact_items[i];
			if(it->press_group != data->bar_id) continue;
			if(it->r.id == data->item_rect.id) cur = n;
			items[n++] = it;
		}
		r_interact* next = items[(cur + (e->keycode == R_KEY_LEFT ? n - 1 : 1)) % n];
		c->is_in_navigation_mode = true;
		r_change_focus(c, next);
		r_ui_list* open = open_menubar_popup(e->ui_list, data->bar_id, next->r.id);
		if(open)
		{
			close_popup(open);
			r_dispatch(next, R_ACTIVATION, &(r_event){ .context = c, .ui_list = e->ui_list });
		}
		return true;
	}
	return false;
}

static bool menubar_hover(const r_event* e, r_callback* self)
{
	struct menubar_data* data = self->data;
	r_ui_list* other = open_menubar_popup(e->ui_list, data->bar_id, data->item_rect.id);
	if(!other) return false;
	close_popup(other);
	r_menu_popup(e->ui_list, data->item_rect, data->menu->submenu, R_BOTTOM);
	return true;
}

static void menubar_item_update(r_context* c, r_drawable* d)
{
	if(popup_is_open(c, d->r, P410)) d->style |= R_STATE_SELECTED;
}

void r_menubar(r_ui_list* l, rect r, const r_menu_item* menus)
{
	r_context* c = l->context;
	r_style(l, r, R_STYLE_MENUBAR_BG);
	rect row = r_inner(c, R_STYLE_MENUBAR_BG, r);
	for(int i = 0; menus[i].label; i++)
	{
		rect inner = r_measure(c, R_STYLE_MENUBAR_ITEM, menus[i].label);
		rect outer = r_outer(c, R_STYLE_MENUBAR_ITEM, &inner);
		rect item = r_cut_px(R_LEFT, &row, outer.w, i);
		r_popup_keep_alive(l, item, P410);
		r_drawable* d = r_drawable_custom(l, r_style(l, item, R_STYLE_MENUBAR_ITEM), (r_data_binding){0}, menubar_item_update, NULL);
		d->text = menus[i].label;
		if(!menus[i].submenu) { r_on_click(l, item, menus[i].on_click); continue; }

		struct menubar_data data = { &menus[i], item, r.id };
		r_callback* act = r_attach_callback(l, item, R_ACTIVATION, menubar_activate);
		r_callback_set_data(l, act, &data, sizeof(data));
		r_callback* hover = r_attach_callback(l, item, R_MOUSE_IN, menubar_hover);
		r_callback_share_data(l, hover, act);
		r_callback* key = r_attach_callback(l, item, R_KEY_INPUT, menubar_key);
		r_callback_share_data(l, key, act);
		r_interact* it = r_interactable(l, item);
		it->press_group = r.id;
		it->traps_arrow_focus = true;
		char ch = menus[i].label[0];
		it->accelerator_key = ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' : ch;
	}
}

struct dropdown_data
{
	char** items;
	int    count;
	r_data_binding binding;
	rect   r;
	int    index;
	r_ui_list* popup;
};

static bool dropdown_select(const r_event* e, r_callback* self)
{
	struct dropdown_data* data = self->data;
	r_seti(data->binding, data->index);
	close_popup(data->popup);
	return true;
}

static bool dropdown_item_key(const r_event* e, r_callback* self)
{
	struct dropdown_data* data = self->data;
	if(e->keycode == R_KEY_UP)   return popup_focus(e->context, data->popup, -1, true);
	if(e->keycode == R_KEY_DOWN) return popup_focus(e->context, data->popup,  1, true);
	return false;
}

static bool dropdown_toggle(const r_event* e, r_callback* self);

static bool dropdown_key(const r_event* e, r_callback* self)
{
	if(e->keycode != R_KEY_DOWN && e->keycode != R_KEY_UP) return false;
	struct dropdown_data* data = self->data;
	if(!r_popup_keep_alive(e->ui_list, data->r, P420))
	{
		e->context->is_in_navigation_mode = true;
		dropdown_toggle(e, self);
	}
	return true;
}

static bool dropdown_toggle(const r_event* e, r_callback* self)
{
	struct dropdown_data* data = self->data;
	r_context* c = e->context;
	r_ui_list* open = r_popup_keep_alive(e->ui_list, data->r, P420);
	if(open) { close_popup(open); return true; }

	r_ui_list* p = r_popup(e->ui_list, data->r, R_POPUP_AUTOCLOSE, P420);
	r_list_clear(p);
	int item_h = r_layout_default_size(c, R_STYLE_MENU_ITEM);
	int w = 0;
	for(int i = 0; i < data->count; i++)
		w = MAX(w, r_measure(c, R_STYLE_MENU_ITEM, data->items[i]).w);
	rect inner = {.w = w, .h = item_h};
	rect item_outer = r_outer(c, R_STYLE_MENU_ITEM, &inner);
	rect list_inner = {.w = item_outer.w, .h = item_h * data->count};
	rect bg = r_outer(c, R_STYLE_MENU_BG, &list_inner);
	bg = r_popup_place(e->ui_list, data->r, MAX(bg.w, data->r.w), bg.h, R_BOTTOM);
	bg.id = p->root_area.id;
	p->root_area = bg;
	r_style(p, bg, R_STYLE_MENU_BG);
	rect area = r_inner(c, R_STYLE_MENU_BG, bg);
	int64_t sel = r_geti(data->binding);
	r_interact* focus = NULL;
	for(int i = 0; i < data->count; i++)
	{
		rect ir = r_cut_px(R_TOP, &area, item_h, i);
		r_drawable* d = r_style(p, ir, R_STYLE_MENU_ITEM | (sel == i ? R_STATE_SELECTED : 0));
		d->text = data->items[i];
		struct dropdown_data item = *data;
		item.index = i; item.popup = p;
		r_callback* cb = r_attach_callback(p, ir, R_ACTIVATION, dropdown_select);
		r_callback_set_data(p, cb, &item, sizeof(item));
		r_callback* key = r_attach_callback(p, ir, R_KEY_INPUT, dropdown_item_key);
		r_callback_share_data(p, key, cb);
		cb->parent->traps_arrow_focus = true;
		if(sel == i) focus = cb->parent;
	}
	if(c->is_in_navigation_mode && focus) r_change_focus(c, focus);
	return true;
}

static void dropdown_update(r_context* c, r_drawable* d)
{
	struct dropdown_data* data = d->dynamic->custom.data;
	if(popup_is_open(c, data->r, P420)) d->style |= R_STATE_TOGGLED;
	int64_t sel = r_geti(data->binding);
	if((d->style & R_STYLE_BASE_MASK) == R_STYLE_TEXT) d->text = sel >= 0 && sel < data->count ? data->items[sel] : "";
}

void r_dropdown(r_ui_list* l, rect r, char** items, int count, r_data_binding selected)
{
	r_context* c = l->context;
	r_popup_keep_alive(l, r, P420);
	struct dropdown_data data = { items, count, selected, r, 0, NULL };
	r_callback* cb = r_attach_callback(l, r, R_ACTIVATION, dropdown_toggle);
	r_callback_set_data(l, cb, &data, sizeof(data));
	r_callback* key = r_attach_callback(l, r, R_KEY_INPUT, dropdown_key);
	r_callback_share_data(l, key, cb);
	if(cb->parent) cb->parent->traps_arrow_focus = true;

	r_drawable_custom(l, r_style(l, r, R_STYLE_DROPDOWN), selected, dropdown_update, cb->data);
	rect inner = r_inner(c, R_STYLE_DROPDOWN, r);
	rect ir = r_cut_px(R_RIGHT, &inner, r_layout_default_size(c, R_STYLE_ICON), P450);
	r_cut_px(R_RIGHT, &inner, r_layout_default_spacing(c, R_STYLE_DROPDOWN) / 2, P450 + 1);
	r_drawable* label = r_label_ex(l, inner, R_STYLE_DROPDOWN, R_ICON_NONE, "");
	label->parent_id = r.id;
	r_drawable_custom(l, label, selected, dropdown_update, cb->data);
	r_drawable* d = r_style(l, ir, R_STYLE_ICON);
	d->icon = R_ICON_DROPDOWN_INDICATOR; d->outer_style = R_STYLE_DROPDOWN; d->parent_id = r.id;
	r_drawable_custom(l, d, selected, dropdown_update, cb->data);
}

rect r_dialog_begin(r_ui_list* d)
{
	r_list_clear(d);
	rect area = {.w = d->root_area.w, .h = d->root_area.h, .id = d->root_area.id};
	r_style(d, area, R_STYLE_DIALOG);
	return r_inner(d->context, R_STYLE_DIALOG, area);
}
