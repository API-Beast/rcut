#include "internal.h"

static inline float stepify(float v, float min, float step)
{
	if(step == 0.0f) return v;
	return min + roundf((v - min) / step) * step;
}

static void format_number(char* buf, int size, const char* prefix, double v, float step)
{
	int decimals = 2;
	if(step >= 1.0f) decimals = 0;
	else if(step > 0.0f) decimals = MIN((int)ceilf(-log10f(step) - 0.001f), 4);
	if(prefix && prefix[0]) snprintf(buf, size, "%s: %.*f", prefix, decimals, v);
	else                    snprintf(buf, size, "%.*f", decimals, v);
}

struct generic_fn_pointer_wrapper_data
{
	void (*fn)();
	r_signature signature;
	uint64_t i;
	double d;
	void* p;
};

static bool generic_fn_pointer_wrapper(const r_event* e, r_callback* self)
{
	struct generic_fn_pointer_wrapper_data* data = self->data;
	r_call(data->signature, data->fn, data->i, data->d, data->p);
	return true;
}

static void on_click_generic(r_ui_list* l, rect r, struct generic_fn_pointer_wrapper_data data)
{
	r_callback* cb = r_attach_callback(l, r, R_ACTIVATION, generic_fn_pointer_wrapper);
	r_callback_set_data(l, cb, &data, sizeof(data));
}

void r_on_click        (r_ui_list* l, rect r, void (*on_click)(void))                   { on_click_generic(l, r, (struct generic_fn_pointer_wrapper_data){.fn = (void(*)())on_click, .signature = R_v_FN}); }
void r_on_click_i32    (r_ui_list* l, rect r, void (*on_click)(int32_t), int32_t value) { on_click_generic(l, r, (struct generic_fn_pointer_wrapper_data){.fn = (void(*)())on_click, .signature = R_v_FN_i32, .i = value}); }
void r_on_click_u64    (r_ui_list* l, rect r, void (*on_click)(uint64_t), uint64_t value){ on_click_generic(l, r, (struct generic_fn_pointer_wrapper_data){.fn = (void(*)())on_click, .signature = R_v_FN_u64, .i = value}); }
void r_on_click_f32    (r_ui_list* l, rect r, void (*on_click)(float), float value)     { on_click_generic(l, r, (struct generic_fn_pointer_wrapper_data){.fn = (void(*)())on_click, .signature = R_v_FN_f32, .d = value}); }
void r_on_click_f64    (r_ui_list* l, rect r, void (*on_click)(double), double value)   { on_click_generic(l, r, (struct generic_fn_pointer_wrapper_data){.fn = (void(*)())on_click, .signature = R_v_FN_f64, .d = value}); }
void r_on_click_ptr    (r_ui_list* l, rect r, void (*on_click)(void*), void* value)     { on_click_generic(l, r, (struct generic_fn_pointer_wrapper_data){.fn = (void(*)())on_click, .signature = R_v_FN_ptr, .p = value}); }
void r_on_click_ptr_i32(r_ui_list* l, rect r, void (*on_click)(void*, int32_t), void* ptr, int index) { on_click_generic(l, r, (struct generic_fn_pointer_wrapper_data){.fn = (void(*)())on_click, .signature = R_v_FN_ptr_i32, .p = ptr, .i = index}); }

r_drawable* r_button_ex(r_ui_list* l, rect r, int style, int icon, const char* text)
{
	r_drawable* d = r_style(l, r, style);
	d->text = (char*)text;
	d->icon = icon;
	r_interactable(l, r);
	return d;
}

void r_button(r_ui_list* l, rect r, const char* text)
{
	r_button_ex(l, r, R_STYLE_BUTTON, R_ICON_NONE, text);
}

r_drawable* r_label_ex(r_ui_list* l, rect r, int style, int icon, const char* text)
{
	r_drawable* d = r_style(l, r, R_STYLE_TEXT);
	d->outer_style = style;
	d->text = (char*)text;
	d->icon = icon;
	return d;
}

void r_label(r_ui_list* l, rect r, const char* text)
{
	r_label_ex(l, r, R_STYLE_LABEL, R_ICON_NONE, text);
}

struct label_data
{
	r_data_binding binding;
	const char* fmt;
	char shown[64];
};

static void format_value(char* buf, int size, const char* fmt, r_data_binding b)
{
	const char* p = fmt;
	while((p = strchr(p, '%')) && p[1] == '%') p += 2;
	if(!p) { snprintf(buf, size, "%s", fmt); return; }
	p += 1 + strspn(p + 1, "-+ #0123456789.");
	char len[2] = {0};
	for(int i = 0; i < 2 && *p && strchr("hljztL", *p); i++) len[i] = *p++;
	if(!*p || !strchr("diouxXcfFeEgGaA", *p)) { snprintf(buf, size, "%s", fmt); return; }
	if(strchr("fFeEgGaA", *p))
	{
		if(len[0] == 'L') snprintf(buf, size, fmt, (long double)r_getd(b));
		else              snprintf(buf, size, fmt, r_getd(b));
		return;
	}
	int64_t v = r_geti(b);
	switch(len[0])
	{
		case 'l': if(len[1] == 'l') snprintf(buf, size, fmt, (long long)v); else snprintf(buf, size, fmt, (long)v); break;
		case 'j': snprintf(buf, size, fmt, (intmax_t)v); break;
		case 'z': case 't': snprintf(buf, size, fmt, (size_t)v); break;
		default: snprintf(buf, size, fmt, (int)v); break;
	}
}

static void label_update(r_context* c, r_drawable* d)
{
	struct label_data* data = d->dynamic->custom.data;
	format_value(data->shown, sizeof(data->shown), data->fmt, data->binding);
	d->text = data->shown;
}

void r_label_value(r_ui_list* l, rect r, const char* fmt, r_data_binding binding)
{
	struct label_data* data = r_storage_alloc(l, sizeof(*data));
	*data = (struct label_data){ binding, fmt };
	format_value(data->shown, sizeof(data->shown), fmt, binding);
	r_drawable_custom(l, r_label_ex(l, r, R_STYLE_LABEL, R_ICON_NONE, data->shown), binding, label_update, data);
}

void r_header(r_ui_list* l, rect r, const char* text)
{
	r_label_ex(l, r, R_STYLE_HEADER, R_ICON_NONE, text);
}

void r_paragraph(r_ui_list* l, rect r, int style, const char* text)
{
	r_label_ex(l, r, style ? style : R_STYLE_LABEL, R_ICON_NONE, text)->wrap_label = true;
}

void r_icon(r_ui_list* l, rect r, int style, int icon)
{
	r_drawable* d = r_style(l, r, R_STYLE_ICON);
	d->outer_style = style;
	d->icon = icon;
}

void r_panel(r_ui_list* l, rect r, int style)
{
	r_style(l, r, style ? style : R_STYLE_PANEL);
}

void r_separator(r_ui_list* l, rect r)
{
	rect line = r;
	if(r.w >= r.h) { line.h = 1; line.y = r.y + r.h / 2; }
	else           { line.w = 1; line.x = r.x + r.w / 2; }
	r_style(l, line, R_STYLE_SEPARATOR | (r.w >= r.h ? R_VARIANT_HORZ : R_VARIANT_VERT));
}

struct toggle_data
{
	r_data_binding binding;
	int64_t value;
	bool    set_value;
};

static bool toggle_activate(const r_event* e, r_callback* self)
{
	struct toggle_data* data = self->data;
	if(data->set_value) r_seti(data->binding, data->value);
	else                r_seti(data->binding, !r_geti(data->binding));
	return true;
}

static r_callback* attach_toggle(r_ui_list* l, rect r, struct toggle_data data)
{
	r_callback* cb = r_attach_callback(l, r, R_ACTIVATION, toggle_activate);
	r_callback_set_data(l, cb, &data, sizeof(data));
	return cb;
}

static r_drawable* show_toggle(r_ui_list* l, r_drawable* d, struct toggle_data t, int state, int icon)
{
	return r_drawable_toggled(l, d, t.binding, t.value, t.set_value, state, icon);
}

static r_drawable* indicator_row(r_ui_list* l, rect r, int style, int indicator_w, const char* text)
{
	r_layout_iter cols = r_layout_columns(l, r, style);
	rect box = {.w = indicator_w ? indicator_w : cols.size, .h = cols.size};
	r_align(R_LEFT_CENTER, r_next(&cols, box.w), &box);
	r_drawable* d = r_style(l, box, style);
	d->parent_id = r.id;
	if(text) r_label_ex(l, r_remaining(&cols), R_STYLE_LABEL, R_ICON_NONE, text)->parent_id = r.id;
	return d;
}

void r_checkbox(r_ui_list* l, rect r, const char* text, r_data_binding binding)
{
	struct toggle_data t = { binding, 0, false };
	attach_toggle(l, r, t);
	show_toggle(l, indicator_row(l, r, R_STYLE_CHECKBOX, 0, text), t, R_STATE_TOGGLED, R_ICON_CHECKED);
}

void r_radio(r_ui_list* l, rect r, const char* text, int64_t value, r_data_binding binding)
{
	struct toggle_data t = { binding, value, true };
	attach_toggle(l, r, t);
	show_toggle(l, indicator_row(l, r, R_STYLE_RADIO, 0, text), t, R_STATE_TOGGLED, R_ICON_MARKED);
}

void r_toggle(r_ui_list* l, rect r, const char* text, r_data_binding binding)
{
	r_context* c = l->context;
	int s = r_layout_default_size(c, R_STYLE_TOGGLE | R_COMPONENT_TRACK);
	int t = r_layout_default_size(c, R_STYLE_TOGGLE | R_COMPONENT_THUMB);
	int inset = (s - t) / 2;
	r_drawable* track = indicator_row(l, r, R_STYLE_TOGGLE | R_COMPONENT_TRACK, s * 9 / 5, text);
	struct toggle_data data = { binding, 0, false };
	attach_toggle(l, r, data);
	show_toggle(l, track, data, R_STATE_TOGGLED, 0);
	rect slide = { track->r.x + inset, track->r.y + inset, track->r.w - s + t, t };
	r_drawable* thumb = r_style(l, (rect){ slide.x, slide.y, t, t }, R_STYLE_TOGGLE | R_COMPONENT_THUMB);
	thumb->parent_id = r.id;
	r_drawable_thumb(l, thumb, binding, 0, 1, slide, R_LEFT);
}

void r_selectable(r_ui_list* l, rect r, const char* text, int64_t value, r_data_binding binding)
{
	r_drawable* d = r_style(l, r, R_STYLE_LIST_ITEM);
	d->text = (char*)text;
	struct toggle_data t = { binding, value, true };
	attach_toggle(l, r, t);
	show_toggle(l, d, t, R_STATE_SELECTED, 0);
}

struct tab_data
{
	struct toggle_data toggle;
	rect pages;
	int  count;
};

static bool tab_activate(const r_event* e, r_callback* self)
{
	struct tab_data* data = self->data;
	toggle_activate(e, self);
	for(int i = 0; i < data->count; i++)
	{
		r_ui_list* page = r_viewport_keep_alive(e->ui_list, r_salt(data->pages, P720 + i));
		if(page) r_list_set_visibility(page, i == data->toggle.value);
	}
	return true;
}

void r_tabs(r_ui_list* l, rect r, char** labels, int count, r_data_binding selected, r_ui_list** pages)
{
	r_context* c = l->context;
	rect bar = r_cut_px(R_TOP, &r, r_layout_default_size(c, R_STYLE_TAB), P670);
	r_style(l, bar, R_STYLE_TAB_BAR);
	r_style(l, r, R_STYLE_TAB_PAGE);
	rect content = r_inner(c, R_STYLE_TAB_PAGE, r);
	rect row = bar;
	for(int i = 0; i < count; i++)
	{
		rect inner = r_measure(c, R_STYLE_TAB, labels[i]);
		rect outer = r_outer(c, R_STYLE_TAB, &inner);
		rect tab = r_cut_px(R_LEFT, &row, outer.w, P670 + i);
		if(i > 0) { tab.x--; tab.w++; }
		tab.h++;
		r_drawable* d = r_style(l, tab, R_STYLE_TAB);
		d->text = labels[i];
		struct tab_data data = { { selected, i, true }, content, count };
		r_callback* cb = r_attach_callback(l, tab, R_ACTIVATION, tab_activate);
		r_callback_set_data(l, cb, &data, sizeof(data));
		show_toggle(l, d, data.toggle, R_STATE_SELECTED, 0);
	}
	int64_t sel = r_geti(selected);
	for(int i = 0; i < count; i++)
	{
		pages[i] = r_viewport(l, r_salt(content, P720 + i));
		pages[i]->document_area.h = content.h;
		pages[i]->is_hidden = i != sel;
	}
}

void r_segmented(r_ui_list* l, rect r, char** labels, int count, r_data_binding selected)
{
	if(count > 1) r_style(l, r, R_STYLE_SEGMENTED | R_COMPONENT_TRACK);
	rect row = r;
	for(int i = 0; i < count; i++)
	{
		rect seg = r_cut_px(R_LEFT, &row, row.w / (count - i), P700 + i);
		if(i > 0) { seg.x--; seg.w++; }
		int comp = count == 1 ? 0 : i == 0 ? R_COMPONENT_ENDCAP_START : i == count - 1 ? R_COMPONENT_ENDCAP_END : R_COMPONENT_JOINED;
		r_drawable* d = r_style(l, seg, R_STYLE_SEGMENTED | comp);
		d->text = labels[i];
		struct toggle_data t = { selected, i, true };
		attach_toggle(l, seg, t);
		show_toggle(l, d, t, R_STATE_TOGGLED, 0);
	}
}

static bool tree_double_click(const r_event* e, r_callback* self)
{
	if(e->click_count != 2 || e->mouse_button != R_MOUSE_LEFT) return false;
	return toggle_activate(e, self);
}

struct tree_data
{
	struct toggle_data select;
	r_data_binding open;
};

static void tree_chevron_update(r_context* c, r_drawable* d)
{
	struct tree_data* t = d->dynamic->custom.data;
	if(r_geti(t->select.binding) == t->select.value) d->style |= R_STATE_SELECTED;
	d->icon = r_geti(t->open) ? R_ICON_HIDE_DETAILS : R_ICON_SHOW_DETAILS;
}

bool r_tree_node(r_ui_list* l, rect r, int depth, const char* text, int icon, bool has_children, r_data_binding open, int64_t value, r_data_binding selected)
{
	r_context* c = l->context;
	bool is_open = has_children && r_geti(open);
	r_drawable* row = r_style(l, r, R_STYLE_LIST_ITEM);
	int is = r_layout_default_size(c, R_STYLE_ICON);
	rect inner = r_inner(c, R_STYLE_LIST_ITEM, r);
	r_cut_px(R_LEFT, &inner, depth * (is + 4), P680);
	rect chevron = r_cut_px(R_LEFT, &inner, is + 4, P680);
	r_drawable* content = r_label_ex(l, inner, R_STYLE_LIST_ITEM, icon, text);
	content->parent_id = r.id;
	if(has_children)
	{
		struct toggle_data data = { open };
		r_callback* dbl = r_attach_callback(l, r, R_ACTIVATION, tree_double_click);
		r_callback_set_data(l, dbl, &data, sizeof(data));
	}
	struct tree_data data = { { selected, value, true }, open };
	r_callback* select = r_attach_callback(l, r, R_ACTIVATION, toggle_activate);
	r_callback_set_data(l, select, &data, sizeof(data));
	show_toggle(l, row, data.select, R_STATE_SELECTED, 0);
	show_toggle(l, content, data.select, R_STATE_SELECTED, 0);
	if(has_children)
	{
		chevron.y = r.y; chevron.h = r.h;
		r_drawable* d = r_label_ex(l, chevron, R_STYLE_LIST_ITEM, R_ICON_NONE, NULL);
		d->parent_id = r.id;
		r_drawable_custom(l, d, open, tree_chevron_update, select->data);
		attach_toggle(l, chevron, (struct toggle_data){ open });
	}
	return is_open;
}

struct split_data
{
	r_data_binding fraction;
	int   total;
	int   min_first, min_second;
	bool  horizontal;
	float start;
};

static int split_size(float fraction, int total, int min_first, int min_second)
{
	int size = (int)(clampf(fraction, 0.0f, 1.0f) * total);
	size = MIN(size, total - min_second);
	return MAX(size, MIN(min_first, total));
}

static bool split_drag(const r_event* e, r_callback* self)
{
	struct split_data* data = self->data;
	if(self->event == R_MOUSE_DRAG_START) { data->start = (float)r_getd(data->fraction); return true; }
	int total = MAX(data->total, 1);
	float delta = data->horizontal ? e->drag_delta_x : e->drag_delta_y;
	float f = (float)split_size(data->start + delta / total, total, data->min_first, data->min_second) / total;
	if(f != (float)r_getd(data->fraction))
		r_setd(data->fraction, f);
	return true;
}

void r_split(r_ui_list* l, rect r, r_alignment dir, r_data_binding fraction, int min_first, int min_second, rect* first, rect* second)
{
	bool horizontal = (dir & (R_LEFT | R_RIGHT)) != 0;
	int gap = r_layout_default_spacing(l->context, R_STYLE_SEPARATOR);
	int total = MAX((horizontal ? r.w : r.h) - gap, 0);
	int size = split_size((float)r_getd(fraction), total, min_first, min_second);
	*second = r;
	*first = r_cut_px(horizontal ? R_LEFT : R_TOP, second, size, P690);
	rect divider = r_cut_px(horizontal ? R_LEFT : R_TOP, second, gap, P690 + 1);
	r_separator(l, divider);

	struct split_data data = { fraction, total, min_first, min_second, horizontal, 0 };
	r_callback* start = r_attach_callback(l, divider, R_MOUSE_DRAG_START, split_drag);
	if(r_copy_last_frame_data(l, start)) data.start = ((struct split_data*)start->data)->start;
	r_callback_set_data(l, start, &data, sizeof(data));
	r_callback* move = r_attach_callback(l, divider, R_MOUSE_DRAG_MOVE, split_drag);
	r_callback_share_data(l, move, start);
	if(start->parent) start->parent->cursor = horizontal ? R_CURSOR_RESIZE_H : R_CURSOR_RESIZE_V;
}

struct slider_data
{
	r_data_binding binding;
	float min, max, step;
	int   thumb;
	bool  vertical;
	rect  r;
	char* text;
	char  shown[64];
};

static void set_value(r_data_binding binding, float min, float max, float step, float v)
{
	v = clampf(stepify(v, min, step), MIN(min, max), MAX(min, max));
	if((float)r_getd(binding) != v) r_setd(binding, v);
}

static void slider_show_value(struct slider_data* data)
{
	format_number(data->shown, sizeof(data->shown), data->text, r_getd(data->binding), data->step);
}

static void slider_set(struct slider_data* data, float v)
{
	set_value(data->binding, data->min, data->max, data->step, v);
	slider_show_value(data);
}

static bool slider_track(const r_event* e, r_callback* self)
{
	struct slider_data* data = self->data;
	rect r = self->parent->r;
	float t;
	if(data->vertical) t = 1.0f - (e->rect_mouse_y - data->thumb * 0.5f) / (float)MAX(r.h - data->thumb, 1);
	else               t =        (e->rect_mouse_x - data->thumb * 0.5f) / (float)MAX(r.w - data->thumb, 1);
	slider_set(data, data->min + clampf(t, 0.0f, 1.0f) * (data->max - data->min));
	return true;
}

static bool slider_key(const r_event* e, r_callback* self)
{
	struct slider_data* data = self->data;
	float step = data->step != 0.0f ? data->step : (data->max - data->min) / 100.0f;
	if(e->modifier_state & R_MOD_SHIFT) step *= 10.0f;
	float v = (float)r_getd(data->binding);
	switch(e->keycode)
	{
		case R_KEY_RIGHT: case R_KEY_UP:   slider_set(data, v + step); return true;
		case R_KEY_LEFT:  case R_KEY_DOWN: slider_set(data, v - step); return true;
		case R_KEY_HOME: slider_set(data, data->min); return true;
		case R_KEY_END:  slider_set(data, data->max); return true;
	}
	return false;
}

static bool slider_wheel(const r_event* e, r_callback* self)
{
	struct slider_data* data = self->data;
	float step = data->step != 0.0f ? data->step : (data->max - data->min) / 100.0f;
	slider_set(data, (float)r_getd(data->binding) + (e->scroll_y > 0 ? step : -step));
	return true;
}

static r_callback* attach_slider(r_ui_list* l, rect r, struct slider_data data, bool wheel)
{
	r_callback* drag = r_attach_callback(l, r, R_MOUSE_DRAG_MOVE, slider_track);
	r_callback_set_data(l, drag, &data, sizeof(data));
	r_callback* key = r_attach_callback(l, r, R_KEY_INPUT, slider_key);
	r_callback_share_data(l, key, drag);
	if(wheel) r_callback_share_data(l, r_attach_callback(l, r, R_MOUSE_SCROLL, slider_wheel), drag);
	if(drag->parent) drag->parent->traps_arrow_focus = true;
	return drag;
}

static void slider_parts(rect r, int ts, bool vertical, float t, rect* track, rect* fill, rect* thumb)
{
	int thickness = MAX(ts / 3, 4);
	*thumb = (rect){.w = ts, .h = ts};
	if(vertical)
	{
		*track = r_shrink_sides(r, (r.w - thickness) / 2, ts / 2, r.w - thickness - (r.w - thickness) / 2, ts / 2);
		r_align_fraction(r, thumb, 0.5f, 1.0f - t);
		*fill = *track; fill->y = thumb->y + ts / 2; fill->h = track->y + track->h - fill->y;
	}
	else
	{
		*track = r_shrink_sides(r, ts / 2, (r.h - thickness) / 2, ts / 2, r.h - thickness - (r.h - thickness) / 2);
		r_align_fraction(r, thumb, t, 0.5f);
		*fill = *track; fill->w = thumb->x + ts / 2 - track->x;
	}
}

static void slider(r_ui_list* l, rect r, bool vertical, float min, float max, float step, r_data_binding binding)
{
	int ts = r_layout_default_size(l->context, R_STYLE_SLIDER | R_COMPONENT_THUMB);
	int orient = vertical ? R_VARIANT_VERT : R_VARIANT_HORZ;
	r_callback* cb = attach_slider(l, r, (struct slider_data){ binding, min, max, step, ts, vertical, r }, false);
	rect track, fill, thumb;
	slider_parts(r, ts, vertical, 0, &track, &fill, &thumb);
	r_style(l, track, R_STYLE_SLIDER | R_COMPONENT_TRACK | orient)->parent_id = r.id;
	r_alignment from = vertical ? R_BOTTOM : R_LEFT;
	r_drawable_fill (l, r_style(l, fill,  R_STYLE_SLIDER | R_COMPONENT_FILL  | orient), binding, min, max, track, from)->parent_id = r.id;
	r_drawable_thumb(l, r_style(l, thumb, R_STYLE_SLIDER | R_COMPONENT_THUMB | orient), binding, min, max, r, from)->parent_id = r.id;
}

void r_slider_h(r_ui_list* l, rect r, float min, float max, float step, r_data_binding binding)
{
	slider(l, r, false, min, max, step, binding);
}

void r_slider_v(r_ui_list* l, rect r, float min, float max, float step, r_data_binding binding)
{
	slider(l, r, true, min, max, step, binding);
}

void r_slider_box(r_ui_list* l, rect r, const char* text, float min, float max, float step, r_data_binding binding)
{
	r_callback* cb = attach_slider(l, r, (struct slider_data){ binding, min, max, step, 0, false, r, (char*)text }, true);
	struct slider_data* data = cb->data;
	slider_show_value(data);
	r_style(l, r, R_STYLE_SLIDEBOX);
	r_drawable_fill(l, r_style(l, r, R_STYLE_SLIDEBOX | R_COMPONENT_FILL), binding, min, max, r_inner(l->context, R_STYLE_SLIDEBOX | R_COMPONENT_FILL, r), R_LEFT)->parent_id = r.id;
	r_label_ex(l, r, R_STYLE_SLIDEBOX, R_ICON_NONE, data->shown)->parent_id = r.id;
}

struct spin_data
{
	r_data_binding binding;
	float min, max, step;
	char  text[64];
};

struct spin_button_data
{
	int direction;
};

static float spin_step(float step, uint32_t modifiers)
{
	return (step != 0.0f ? step : 1.0f) * (modifiers & R_MOD_SHIFT ? 10.0f : 1.0f);
}

static void spin_show_value(const r_event* e, r_callback* self, struct spin_data* d)
{
	format_number(d->text, sizeof(d->text), NULL, r_getd(d->binding), d->step);
	if(e) r_damage_id(e->context, self->parent->r.id);
}

static void spin_commit(const r_event* e, r_callback* self, struct spin_data* d)
{
	char* end;
	double v = strtod(d->text, &end);
	if(end != d->text) set_value(d->binding, d->min, d->max, d->step, (float)v);
	spin_show_value(e, self, d);
}

static bool spin_key(const r_event* e, r_callback* self)
{
	struct spin_data* d = self->data;
	if(e->keycode != R_KEY_UP && e->keycode != R_KEY_DOWN) return false;
	float step = spin_step(d->step, e->modifier_state);
	set_value(d->binding, d->min, d->max, d->step, (float)r_getd(d->binding) + (e->keycode == R_KEY_UP ? step : -step));
	spin_show_value(e, self, d);
	return true;
}

static bool spin_wheel(const r_event* e, r_callback* self)
{
	struct spin_data* d = self->data;
	float step = spin_step(d->step, 0);
	set_value(d->binding, d->min, d->max, d->step, (float)r_getd(d->binding) + (e->scroll_y > 0 ? step : -step));
	spin_show_value(e, self, d);
	return true;
}

static bool spin_enter_or_leave(const r_event* e, r_callback* self)
{
	if(self->event == R_ACTIVATION && e->keycode != R_KEY_ENTER) return false;
	spin_commit(e, self, self->data);
	return false;
}

static bool spin_press(const r_event* e, r_callback* self)
{
	struct spin_button_data* b = self->data;
	if(e->mouse_button != R_MOUSE_LEFT) return false;
	r_interact* field = r_list_find_this_frame_interact(e->ui_list, self->parent->r.id);
	struct spin_data* d = NULL;

	for(r_callback* cb = field ? field->callbacks : NULL; cb && !d; cb = cb->next)
	if(cb->function == spin_key)
		d = cb->data;

	if(!d) return false;
	spin_commit(e, self, d);
	set_value(d->binding, d->min, d->max, d->step, (float)r_getd(d->binding) + b->direction * spin_step(d->step, 0));
	spin_show_value(e, self, d);
	return true;
}

void r_spinbox(r_ui_list* l, rect r, float min, float max, float step, r_data_binding binding)
{
	r_style(l, r, R_STYLE_SEGMENTED | R_COMPONENT_TRACK);
	int bw = r.h;
	rect plus  = r_cut_px(R_RIGHT, &r, bw, P640);
	rect minus = r_cut_px(R_RIGHT, &r, bw, P630);
	plus.x--; plus.w++;
	minus.x--; minus.w++;

	struct spin_data data = { binding, min, max, step, "" };
	r_callback* key = r_attach_callback(l, r, R_KEY_INPUT, spin_key);
	if(l->context->focus_id == r.id && r_copy_last_frame_data(l, key)) memcpy(data.text, ((struct spin_data*)key->data)->text, sizeof(data.text));
	else spin_show_value(NULL, NULL, &data);
	r_callback_set_data(l, key, &data, sizeof(data));
	r_event_type shared[] = { R_MOUSE_SCROLL, R_ACTIVATION, R_FOCUS_EXIT };
	r_callback_fn* fns[] = { spin_wheel, spin_enter_or_leave, spin_enter_or_leave };
	for(int i = 0; i < 3; i++) r_callback_share_data(l, r_attach_callback(l, r, shared[i], fns[i]), key);
	struct spin_data* d = key->data;
	r_text_edit_ex(l, r, d->text, sizeof(d->text), NULL, R_STYLE_SPINBOX, false);

	rect buttons[2] = { minus, plus };
	int styles[2] = { R_STYLE_SEGMENTED | R_COMPONENT_JOINED, R_STYLE_SEGMENTED | R_COMPONENT_ENDCAP_END };
	for(int i = 0; i < 2; i++)
	{
		buttons[i].id = r.id;
		r_style(l, buttons[i], styles[i])->icon = i ? R_ICON_ADD : R_ICON_SUBTRACT;
		struct spin_button_data button = { i ? 1 : -1 };
		r_callback* cb = r_attach_callback_to(l, r_interactable_new(l, buttons[i]), R_MOUSE_PRESSED, spin_press);
		r_callback_set_data(l, cb, &button, sizeof(button));
	}
}

void r_progress(r_ui_list* l, rect r, float fraction)
{
	fraction = clampf(fraction, 0.0f, 1.0f);
	r = r_offset(r, 0, 1);
	r_style(l, r, R_STYLE_PROGRESS | R_COMPONENT_TRACK);
	rect fill = r_inner(l->context, R_STYLE_PROGRESS | R_COMPONENT_FILL, r);
	fill.w = (int)(fill.w * fraction + 0.5f);
	if(fill.w > 0) r_style(l, fill, R_STYLE_PROGRESS | R_COMPONENT_FILL);
}

struct scroll_data
{
	r_ui_list* viewport;
	float max_scroll;
	int   track_y, track_h, thumb_h;
	float drag_start;
};

void r_scroll_to(r_ui_list* v, float x, float y)
{
	float max_x = MAX(v->document_area.w - v->root_area.w, 0);
	float max_y = MAX(v->document_area.h - v->root_area.h, 0);
	x = clampf(x, 0, max_x); y = clampf(y, 0, max_y);
	if(x == v->scroll_x && y == v->scroll_y) return;
	v->scroll_x = x; v->scroll_y = y;
	v->needs_redraw = true;
}

static bool scroll_wheel(const r_event* e, r_callback* self)
{
	struct scroll_data* data = self->data;
	if(data->max_scroll <= 0) return false;
	int line = r_measure(e->context, R_STYLE_TEXT, "").h;
	r_scroll_to(data->viewport, data->viewport->scroll_x, data->viewport->scroll_y - e->scroll_y * line * 3);
	return true;
}

struct scroll_button_data { r_ui_list* viewport; int direction; };

static bool scroll_button(const r_event* e, r_callback* self)
{
	struct scroll_button_data* b = self->data;
	if(e->mouse_button != R_MOUSE_LEFT) return false;
	int line = r_measure(e->context, R_STYLE_TEXT, "").h;
	r_scroll_to(b->viewport, b->viewport->scroll_x, b->viewport->scroll_y + b->direction * line * 2);
	return true;
}

static bool scroll_thumb_drag(const r_event* e, r_callback* self)
{
	struct scroll_data* data = self->data;
	if(self->event == R_MOUSE_DRAG_START) { data->drag_start = data->viewport->scroll_y; return true; }
	float range = (float)MAX(data->track_h - data->thumb_h, 1);
	r_scroll_to(data->viewport, data->viewport->scroll_x, data->drag_start + e->drag_delta_y * data->max_scroll / range);
	return true;
}

static bool scroll_track_press(const r_event* e, r_callback* self)
{
	struct scroll_data* data = self->data;
	float range = (float)MAX(data->track_h - data->thumb_h, 1);
	r_scroll_to(data->viewport, data->viewport->scroll_x, (e->rect_mouse_y - data->thumb_h * 0.5f) / range * data->max_scroll);
	return true;
}

r_ui_list* r_scroll_begin(r_ui_list* parent, rect r)
{
	return r_scroll_begin_ex(parent, r, R_STYLE_PANEL);
}

r_ui_list* r_scroll_begin_ex(r_ui_list* parent, rect r, int frame_style)
{
	r_context* c = parent->context;
	rect vr = r;
	if(frame_style)
	{
		r_style(parent, r, frame_style);
		vr = r_inner(c, frame_style, r);
	}
	vr.id = r_salt(r, P650).id;
	vr.w -= r_layout_default_size(c, R_STYLE_SCROLLBAR | R_COMPONENT_TRACK);
	return r_viewport(parent, vr);
}

void r_scroll_end(r_ui_list* parent, r_ui_list* v)
{
	r_context* c = parent->context;
	rect content = r_list_get_content_rect(v);
	rect vr = v->root_area;
	int doc_h = MAX(v->document_area.h, content.y + content.h);
	if(v->count_draw_items == 0) doc_h = v->document_area.h;
	v->document_area.h = doc_h;
	v->has_scrollbar = doc_h > vr.h;

	struct scroll_data data = { v, (float)MAX(doc_h - vr.h, 0) };
	v->scroll_y = clampf(v->scroll_y, 0, data.max_scroll);

	int sbw = r_layout_default_size(c, R_STYLE_SCROLLBAR | R_COMPONENT_TRACK);
	int cap = r_layout_default_size(c, R_STYLE_SCROLLBAR | R_COMPONENT_ENDCAP_START);
	cap = cap > 0 ? MIN(cap, vr.h / 3) : 0;
	rect track = {.x = vr.x + vr.w, .y = vr.y + cap, .w = sbw, .h = vr.h - cap * 2, .id = r_salt(vr, P660).id};
	rect thumb = track;
	data.track_y = track.y;
	data.track_h = track.h;
	data.thumb_h = MIN(MAX(track.h * vr.h / MAX(doc_h, 1), sbw * 2), track.h);
	thumb.h = data.thumb_h;
	thumb.id = r_salt(track, P600).id;

	rect whole = vr;
	whole.w += sbw;
	r_callback* wheel = r_attach_callback(parent, whole, R_MOUSE_SCROLL, scroll_wheel);
	r_callback_set_data(parent, wheel, &data, sizeof(data));

	if(!v->has_scrollbar) // Empty gutter.
		r_style(parent, (rect){ track.x, vr.y, sbw, vr.h, track.id }, R_STYLE_SCROLLBAR | R_VARIANT_VERT | R_COMPONENT_TRACK);
	else
	{
		r_style(parent, track, R_STYLE_SCROLLBAR | R_VARIANT_VERT | R_COMPONENT_TRACK);
		r_drawable* thumb_d = r_drawable_thumb(parent, r_style(parent, thumb, R_STYLE_SCROLLBAR | R_VARIANT_VERT | R_COMPONENT_THUMB), R_PTR(&v->scroll_y), 0, data.max_scroll, track, R_TOP);

		for(int i = 0; cap && i < 2; i++)
		{
			rect button = { track.x, i ? track.y + track.h : vr.y, sbw, cap, r_salt(track, P610 + i).id };
			r_style(parent, button, R_STYLE_SCROLLBAR | R_VARIANT_VERT | (i ? R_COMPONENT_ENDCAP_END : R_COMPONENT_ENDCAP_START))->icon = i ? R_ICON_DOWN : R_ICON_UP;
			struct scroll_button_data b = { v, i ? 1 : -1 };
			r_callback* cb = r_attach_callback(parent, button, R_MOUSE_PRESSED, scroll_button);
			r_callback_set_data(parent, cb, &b, sizeof(b));
		}

		r_callback* press = r_attach_callback(parent, track, R_MOUSE_PRESSED, scroll_track_press);
		r_callback_share_data(parent, press, wheel);
		r_callback* start = r_attach_callback(parent, thumb, R_MOUSE_DRAG_START, scroll_thumb_drag);
		start->parent->dynamic = thumb_d->dynamic;
		if(r_copy_last_frame_data(parent, start)) data.drag_start = ((struct scroll_data*)start->data)->drag_start;
		r_callback_set_data(parent, start, &data, sizeof(data));
		r_callback* move = r_attach_callback(parent, thumb, R_MOUSE_DRAG_MOVE, scroll_thumb_drag);
		r_callback_share_data(parent, move, start);
	}
}
