#include "internal.h"

static inline int sel_min(r_text_edit_state* s){ return MIN(s->caret, s->anchor); }
static inline int sel_max(r_text_edit_state* s){ return MAX(s->caret, s->anchor); }

static int line_start(const char* text, int i) { while(i > 0 && text[i - 1] != '\n') i--; return i; }
static int line_end  (const char* text, int i) { while(text[i] && text[i] != '\n') i++; return i; }
static int line_of   (const char* text, int i) { int n = 0; for(int k = 0; k < i; k++) n += text[k] == '\n'; return n; }

static int nth_line(const char* text, int n)
{
	int i = 0;
	while(n > 0 && text[i]) { if(text[i] == '\n') n--; i++; }
	return line_start(text, i);
}

static void caret_pos(r_context* c, r_text_edit_state* s, int i, int* x, int* y)
{
	int ls = line_start(s->buffer, i);
	*x = r_measure_n(c, s->style, s->buffer + ls, i - ls).w;
	*y = line_of(s->buffer, i) * r_measure(c, s->style, "").h;
}

static int column_at(r_context* c, r_text_edit_state* s, int ls, int x)
{
	int le = line_end(s->buffer, ls);
	int best = ls, best_dist = x < 0 ? -x : x;
	for(int i = ls; i < le; )
	{
		i = utf8_next(s->buffer, i);
		int d = r_measure_n(c, s->style, s->buffer + ls, i - ls).w - x;
		if(d < 0) d = -d;
		if(d < best_dist) { best = i; best_dist = d; }
		else if(i > best) break;
	}
	return best;
}

static int index_at(r_context* c, r_text_edit_state* s, int x, int y)
{
	int line = s->multiline ? MAX(y, 0) / MAX(r_measure(c, s->style, "").h, 1) : 0;
	return column_at(c, s, nth_line(s->buffer, line), x);
}

static void delete_range(r_text_edit_state* s, int a, int b)
{
	if(a >= b) return;
	int len = (int)strlen(s->buffer);
	memmove(s->buffer + a, s->buffer + b, len - b + 1);
	s->caret = s->anchor = a;
}

static void delete_selection(r_text_edit_state* s)
{
	delete_range(s, sel_min(s), sel_max(s));
}

static void insert_text(r_text_edit_state* s, const char* text)
{
	delete_selection(s);
	int len = (int)strlen(s->buffer);
	int n = (int)strlen(text);
	if(len + n > s->capacity - 1) n = s->capacity - 1 - len;
	while(n > 0 && (text[n] & 0xC0) == 0x80) n--;
	if(n <= 0) return;
	memmove(s->buffer + s->caret + n, s->buffer + s->caret, len - s->caret + 1);
	memcpy(s->buffer + s->caret, text, n);
	s->caret += n;
	s->anchor = s->caret;
}

static bool is_word_char(char ch)
{
	return ch != ' ' && ch != '\t' && ch != '\n' && ch != '.' && ch != ',' && ch != '/' && ch != '-' && ch != '_' && ch != ':' && ch != ';';
}

static int word_step(const char* text, int i, int dir)
{
	if(dir > 0)
	{
		while(text[i] && !is_word_char(text[i])) i = utf8_next(text, i);
		while(text[i] &&  is_word_char(text[i])) i = utf8_next(text, i);
	}
	else
	{
		while(i > 0 && !is_word_char(text[i - 1])) i = utf8_prev(text, i);
		while(i > 0 &&  is_word_char(text[i - 1])) i = utf8_prev(text, i);
	}
	return i;
}

static void follow_caret(r_context* c, r_text_edit_state* s)
{
	int len = (int)strlen(s->buffer);
	s->caret  = MIN(MAX(s->caret, 0), len);
	s->anchor = MIN(MAX(s->anchor, 0), len);
	rect inner = s->inner;
	int line_h = r_measure(c, s->style, "").h;
	if(s->caret != s->last_caret)
	{
		int caret_x, caret_y;
		caret_pos(c, s, s->caret, &caret_x, &caret_y);
		if(caret_x - s->scroll_x > inner.w - 2) s->scroll_x = caret_x - inner.w + 2;
		if(caret_x - s->scroll_x < 0)           s->scroll_x = caret_x;
		if(s->multiline)
		{
			if(caret_y + line_h - s->scroll_y > inner.h) s->scroll_y = caret_y + line_h - inner.h;
			if(caret_y - s->scroll_y < 0)                s->scroll_y = caret_y;
		}
		s->last_caret = s->caret;
	}
	rect text_size = r_measure_n(c, s->style, s->buffer, -1);
	s->scroll_x = MAX(0, MIN(s->scroll_x, text_size.w - inner.w + 2));
	s->scroll_y = s->multiline ? MAX(0, MIN(s->scroll_y, text_size.h - inner.h)) : 0;
}

static void edited(const r_event* e, r_callback* self)
{
	follow_caret(e->context, self->data);
	e->context->time_since_focus = 0;
	r_damage_id(e->context, self->parent->r.id);
}

static bool edit_text_input(const r_event* e, r_callback* self)
{
	r_text_edit_state* s = self->data;
	insert_text(s, e->text);
	s->preferred_x = -1;
	edited(e, self);
	return true;
}

static void move_lines(r_context* c, r_text_edit_state* s, int lines)
{
	int x, y;
	caret_pos(c, s, s->caret, &x, &y);
	if(s->preferred_x < 0) s->preferred_x = x;
	int line = line_of(s->buffer, s->caret) + lines;
	if(line < 0) { s->caret = 0; return; }
	int total = line_of(s->buffer, (int)strlen(s->buffer));
	if(line > total) { s->caret = (int)strlen(s->buffer); return; }
	s->caret = column_at(c, s, nth_line(s->buffer, line), s->preferred_x);
}

static bool edit_key(const r_event* e, r_callback* self)
{
	r_text_edit_state* s = self->data;
	r_context* c = e->context;
	bool shift = e->modifier_state & R_MOD_SHIFT;
	bool ctrl  = e->modifier_state & R_MOD_CTRL;
	int len = (int)strlen(s->buffer);
	int i = s->caret;
	bool keep_column = false;
	switch(e->keycode)
	{
		case R_KEY_LEFT:
		case R_KEY_RIGHT:
		{
			int dir = e->keycode == R_KEY_LEFT ? -1 : 1;
			if(!shift && s->caret != s->anchor && !ctrl)
				i = dir < 0 ? sel_min(s) : sel_max(s);
			else if(ctrl) i = word_step(s->buffer, i, dir);
			else i = dir < 0 ? utf8_prev(s->buffer, i) : utf8_next(s->buffer, i);
			s->caret = i;
			break;
		}
		case R_KEY_UP: case R_KEY_DOWN: case R_KEY_PAGEUP: case R_KEY_PAGEDOWN:
		{
			bool up = e->keycode == R_KEY_UP || e->keycode == R_KEY_PAGEUP;
			if(!s->multiline) { s->caret = up ? 0 : len; break; }
			int lines = e->keycode == R_KEY_UP || e->keycode == R_KEY_DOWN ? 1 : 10;
			move_lines(c, s, up ? -lines : lines);
			keep_column = true;
			break;
		}
		case R_KEY_HOME: s->caret = (ctrl || !s->multiline) ? 0   : line_start(s->buffer, i); break;
		case R_KEY_END:  s->caret = (ctrl || !s->multiline) ? len : line_end(s->buffer, i);   break;
		case R_KEY_BACKSPACE:
			if(s->caret == s->anchor)
			{
				if(ctrl) i = word_step(s->buffer, i, -1); else i = utf8_prev(s->buffer, i);
				delete_range(s, i, s->caret);
			}
			else delete_selection(s);
			break;
		case R_KEY_DELETE:
			if(s->caret == s->anchor)
			{
				if(ctrl) i = word_step(s->buffer, i, 1); else i = utf8_next(s->buffer, i);
				delete_range(s, s->caret, i);
			}
			else delete_selection(s);
			break;
		case 'A':
			if(!ctrl) return false;
			s->anchor = 0; s->caret = len;
			break;
		case 'C': case 'X':
		{
			if(!ctrl) return false;
			if(s->caret == s->anchor || !c->interface.set_clipboard) return true;
			int a = sel_min(s), b = sel_max(s);
			char tmp = s->buffer[b];
			s->buffer[b] = 0;
			c->interface.set_clipboard(c, s->buffer + a);
			s->buffer[b] = tmp;
			if(e->keycode == 'X') delete_selection(s);
			break;
		}
		case 'V':
		{
			if(!ctrl) return false;
			char* clip = c->interface.get_clipboard ? c->interface.get_clipboard(c) : NULL;
			if(clip)
			{
				if(!s->multiline) { char* nl = strpbrk(clip, "\r\n"); if(nl) *nl = 0; }
				insert_text(s, clip);
			}
			break;
		}
		default: return false;
	}
	bool moves = e->keycode != R_KEY_BACKSPACE && e->keycode != R_KEY_DELETE && e->keycode != 'X' && e->keycode != 'V' && e->keycode != 'C' && e->keycode != 'A';
	if(moves && !shift) s->anchor = s->caret;
	if(!keep_column) s->preferred_x = -1;
	edited(e, self);
	return true;
}

static bool edit_mouse(const r_event* e, r_callback* self)
{
	r_text_edit_state* s = self->data;
	if(e->mouse_button != R_MOUSE_LEFT) return false;
	rect inner = r_inner(e->context, s->style, self->parent->r);
	s->caret = index_at(e->context, s, (int)e->list_mouse_x - inner.x + s->scroll_x, (int)e->list_mouse_y - inner.y + s->scroll_y);
	if(self->event == R_MOUSE_PRESSED && !(e->modifier_state & R_MOD_SHIFT))
		s->anchor = s->caret;
	if(self->event == R_MOUSE_PRESSED && e->click_count == 2)
	{
		int a = s->caret, b = s->caret;
		bool word = is_word_char(s->buffer[a]) || (a > 0 && is_word_char(s->buffer[a - 1]));
		while(a > 0 && s->buffer[a - 1] != '\n' && is_word_char(s->buffer[a - 1]) == word) a--;
		while(s->buffer[b] && s->buffer[b] != '\n' && is_word_char(s->buffer[b]) == word) b++;
		s->anchor = a; s->caret = b;
	}
	else if(self->event == R_MOUSE_PRESSED && e->click_count >= 3)
	{
		s->anchor = line_start(s->buffer, s->caret);
		s->caret = line_end(s->buffer, s->caret);
	}
	s->preferred_x = -1;
	edited(e, self);
	return true;
}

static bool edit_wheel(const r_event* e, r_callback* self)
{
	r_text_edit_state* s = self->data;
	if(!s->multiline) return false;
	int line_h = r_measure(e->context, s->style, "").h;
	rect inner = r_inner(e->context, s->style, self->parent->r);
	int max_scroll = MAX(r_measure_n(e->context, s->style, s->buffer, -1).h - inner.h, 0);
	int scroll = MAX(0, MIN(s->scroll_y - (int)(e->scroll_y * line_h * 3), max_scroll));
	if(scroll == s->scroll_y) return false;
	s->scroll_y = scroll;
	return true;
}

static bool edit_focus(const r_event* e, r_callback* self)
{
	r_text_edit_state* s = self->data;
	if(self->event == R_FOCUS_ENTER && e->context->is_in_navigation_mode && !s->multiline)
	{
		s->anchor = 0;
		s->caret = (int)strlen(s->buffer);
	}
	edited(e, self);
	return false;
}

static bool edit_activate(const r_event* e, r_callback* self)
{
	r_text_edit_state* s = self->data;
	if(e->keycode != R_KEY_ENTER) return true;
	if(s->multiline && !(e->modifier_state & R_MOD_CTRL))
	{
		insert_text(s, "\n");
		s->preferred_x = -1;
	}
	else if(s->on_submit) s->on_submit(s->buffer);
	edited(e, self);
	return true;
}

static int text_top(r_text_edit_state* s, int line_h)
{
	return s->multiline ? s->inner.y : s->inner.y + (s->inner.h - line_h) / 2;
}

static void edit_text_update(r_context* c, r_drawable* d)
{
	r_text_edit_state* s = d->dynamic->custom.data;
	bool placeholder = !s->buffer[0] && s->placeholder;
	d->text = placeholder ? s->placeholder : s->buffer;
	d->outer_style = s->style | (placeholder ? R_VARIANT_DULL : 0);
	d->scroll_x = (int16_t)s->scroll_x;
	d->scroll_y = (int16_t)s->scroll_y;
}

static void edit_caret_update(r_context* c, r_drawable* d)
{
	r_text_edit_state* s = d->dynamic->custom.data;
	rect caret = {0};
	if(c->focus_id == d->parent_id)
	{
		int line_h = r_measure(c, s->style, "").h, x, y;
		caret_pos(c, s, MIN(s->caret, (int)strlen(s->buffer)), &x, &y);
		caret = rect_intersect((rect){ s->inner.x + x - s->scroll_x, text_top(s, line_h) + y - s->scroll_y, 2, line_h }, r_grow(s->inner, 1));
	}
	d->r = (rect){ caret.x, caret.y, caret.w, caret.h, d->r.id };
}

static void edit_selection_update(r_context* c, r_drawable* d, int part)
{
	r_text_edit_state* s = d->dynamic->custom.data;
	rect sel = {0};
	int len = c->focus_id == d->parent_id ? (int)strlen(s->buffer) : 0, a = MIN(sel_min(s), len), b = MIN(sel_max(s), len);
	if(a != b)
	{
		int first = line_of(s->buffer, a), last = line_of(s->buffer, b), y;
		int x0 = 0, x1 = s->scroll_x + s->inner.w, top = first, bottom = first + 1;
		if(part == 0) caret_pos(c, s, a, &x0, &y);
		if(part == 0 && first == last) caret_pos(c, s, b, &x1, &y);
		if(part == 1) { top = first + 1; bottom = last; }
		if(part == 2) { top = last; bottom = first == last ? last : last + 1; caret_pos(c, s, b, &x1, &y); }
		int line_h = r_measure(c, s->style, "").h;
		sel = (rect){ s->inner.x + x0 - s->scroll_x, text_top(s, line_h) + top * line_h - s->scroll_y, x1 - x0, (bottom - top) * line_h };
		sel = sel.h > 0 ? rect_intersect(sel, s->inner) : (rect){0};
	}
	d->r = (rect){ sel.x, sel.y, sel.w, sel.h, d->r.id };
}

static void edit_selection_first (r_context* c, r_drawable* d) { edit_selection_update(c, d, 0); }
static void edit_selection_middle(r_context* c, r_drawable* d) { edit_selection_update(c, d, 1); }
static void edit_selection_last  (r_context* c, r_drawable* d) { edit_selection_update(c, d, 2); }

void r_text_edit_ex(r_ui_list* l, rect r, char* buffer, int capacity, char* placeholder, int style, bool multiline)
{
	r_context* c = l->context;
	r_callback* input = r_attach_callback(l, r, R_TEXT_INPUT, edit_text_input);
	r_callback_set_data(l, input, NULL, sizeof(r_text_edit_state));
	bool is_new = !r_copy_last_frame_data(l, input);
	r_text_edit_state* s = input->data;
	if(is_new) { s->preferred_x = -1; s->last_caret = -1; }
	s->buffer = buffer; s->capacity = capacity; s->style = style; s->multiline = multiline;
	s->placeholder = placeholder;
	s->inner = r_inner(c, style, r);
	follow_caret(c, s);

	r_style(l, r, style);
	int styles[] = { style | R_COMPONENT_SELECTION, style | R_COMPONENT_SELECTION, style | R_COMPONENT_SELECTION, R_STYLE_TEXT, style | R_COMPONENT_CARET };
	void (*update[])(r_context* c, r_drawable* d) = { edit_selection_first, edit_selection_middle, edit_selection_last, edit_text_update, edit_caret_update };
	for(int i = 0; i < COUNTOF(styles); i++)
	{
		r_drawable* d = r_style(l, s->inner, styles[i]);
		d->parent_id = r.id;
		r_drawable_custom(l, d, (r_data_binding){0}, update[i], s);
	}

	r_callback* cbs[] = {
		r_attach_callback(l, r, R_KEY_INPUT,       edit_key),
		r_attach_callback(l, r, R_MOUSE_PRESSED,   edit_mouse),
		r_attach_callback(l, r, R_MOUSE_DRAG_MOVE, edit_mouse),
		r_attach_callback(l, r, R_MOUSE_SCROLL,    edit_wheel),
		r_attach_callback(l, r, R_ACTIVATION,      edit_activate),
		r_attach_callback(l, r, R_FOCUS_ENTER,     edit_focus),
		r_attach_callback(l, r, R_FOCUS_EXIT,      edit_focus),
	};
	for(int i = 0; i < COUNTOF(cbs); i++) r_callback_share_data(l, cbs[i], input);
	if(input->parent) { input->parent->traps_arrow_focus = true; input->parent->cursor = R_CURSOR_TEXT; }
}

void r_line_edit(r_ui_list* l, rect r, char* buffer, int capacity, char* placeholder)
{
	r_text_edit_ex(l, r, buffer, capacity, placeholder, R_STYLE_INPUT, false);
}

void r_text_area(r_ui_list* l, rect r, char* buffer, int capacity, char* placeholder)
{
	r_text_edit_ex(l, r, buffer, capacity, placeholder, R_STYLE_INPUT_MULTILINE_EDIT, true);
}

void r_line_edit_on_submit(r_ui_list* l, rect r, void (*on_submit)(char* buffer))
{
	r_interact* it = r_list_find_this_frame_interact(l, r.id);
	for(r_callback* cb = it ? it->callbacks : NULL; cb; cb = cb->next)
		if(cb->function == edit_text_input) ((r_text_edit_state*)cb->data)->on_submit = on_submit;
}
