#include "internal.h"

static inline bool stack_horizontal(r_layout_iter* s) { return (s->dir & (R_LEFT | R_RIGHT)) != 0; }
static inline bool is_flow(r_layout_iter* s) { return s->dir == R_TOP_LEFT; }

int r_layout_default_size(r_context* c, int style)
{
	int size = 28, spacing = 8;
	if(c->interface.get_layout) c->interface.get_layout(c, style, &size, &spacing);
	return size;
}

int r_layout_default_spacing(r_context* c, int style)
{
	int size = 28, spacing = 8;
	if(c->interface.get_layout) c->interface.get_layout(c, style, &size, &spacing);
	return spacing;
}

r_layout_iter r_layout(r_ui_list* l, rect area, r_alignment dir, int style)
{
	r_layout_iter s = { .l = l, .start = area, .area = area, .dir = dir };
	s.size    = r_layout_default_size(l->context, style);
	s.spacing = r_layout_default_spacing(l->context, style);
	return s;
}

r_layout_iter r_stack(r_ui_list* l, rect area, r_alignment dir)
{
	return r_layout(l, area, dir, R_STYLE_FALLBACK);
}

static r_layout_iter stack_nested(r_layout_iter* parent, rect area, r_alignment dir)
{
	return (r_layout_iter){ .l = parent->l, .start = area, .area = area, .dir = dir, .size = parent->size, .spacing = parent->spacing };
}

static inline uint64_t stack_id(r_layout_iter* s, uint64_t salt)
{
	return xxhash64(s->start.id, (uint64_t)s->count, salt, 0);
}

static void flow_newline(r_layout_iter* s, int min_h)
{
	r_ui_list* l = s->l;
	int h = MAX(s->line_h, min_h), text_h = 0;

	for(int i = s->line_first; i < l->count_draw_items; i++)
	if(l->draw_items[i].style == R_STYLE_TEXT && !l->draw_items[i].parent_id)
		text_h = MAX(text_h, l->draw_items[i].r.h);

	for(int i = s->line_first; i < l->count_draw_items; i++)
	{
		r_drawable* d = &l->draw_items[i];
		if(d->parent_id) continue;
		if(d->style == R_STYLE_TEXT) d->r.y = s->area.y + (h - text_h) / 2 + text_h - d->r.h;
		if(d->style == R_STYLE_ICON) d->r.y = s->area.y + (h - d->r.h) / 2;
	}
	s->area = (rect){ s->start.x, s->area.y + h, s->start.w, s->area.h - h, s->area.id };
	s->line_h = 0;
	s->line_first = l->count_draw_items;
}

static rect flow_item(r_layout_iter* s, int w, int h)
{
	rect r = { s->area.x, s->area.y, w, h, stack_id(s, P800) };
	s->count++;
	s->area.x += w; s->area.w -= w;
	s->line_h = MAX(s->line_h, h);
	return r;
}

static rect flow_next(r_layout_iter* s, int w, int h)
{
	if(w > s->area.w && s->area.x > s->start.x) flow_newline(s, 0);
	return flow_item(s, w, h);
}

r_layout_iter r_flow_begin(r_layout_iter* parent, int style)
{
	r_layout_iter s = r_group_begin(parent, 0);
	s.dir = R_TOP_LEFT;
	s.style = style;
	s.size = r_measure(parent->l->context, style, "").h;
	s.spacing = 0;
	s.line_first = parent->l->count_draw_items;
	return s;
}

void r_text(r_layout_iter* s, int style, char* text)
{
	r_ui_list* l = s->l;
	if(!style) style = s->style;
	int h = r_measure(l->context, style, "").h;
	for(int pos = 0, len = (int)strlen(text); pos < len; )
	{
		if(text[pos] == '\n') { flow_newline(s, h); pos++; continue; }
		bool line_empty = s->area.x == s->start.x;
		int w, n = r_break_text(l->context, style, text + pos, len - pos, s->area.w, line_empty, &w);
		if(n <= 0)
		{
			if(line_empty) break;
			flow_newline(s, 0);
			continue;
		}
		char* fragment = r_store(l, text + pos, n + 1);
		fragment[n] = 0;
		r_label_ex(l, flow_item(s, w, h), style, R_ICON_NONE, fragment);
		pos += n;
		if(pos < len && text[pos] != '\n') flow_newline(s, 0);
	}
}

rect r_flow_end(r_layout_iter* s, r_layout_iter* parent)
{
	if(s->area.x > s->start.x) flow_newline(s, 0);
	return r_group_end(s, parent);
}

rect r_next(r_layout_iter* s, int size)
{
	if(is_flow(s)) return flow_next(s, size < 0 ? s->size : size, s->size);
	if(s->count > 0) r_cut_px(s->dir, &s->area, s->spacing, 0);
	if(size < 0) size = s->size;
	rect r = r_cut_px(s->dir, &s->area, size, 0);
	r.id = stack_id(s, P800);
	s->count++;
	return r;
}

rect r_next_keyed(r_layout_iter* s, int size, uint64_t key)
{
	rect r = r_next(s, size);
	r.id = xxhash64(s->start.id, key, P800, 1);
	return r;
}

rect r_next_fraction(r_layout_iter* s, float fraction)
{
	int len = stack_horizontal(s) ? s->start.w : s->start.h;
	return r_next(s, (int)((len - s->spacing) * fraction));
}

rect r_next_fit(r_layout_iter* s, int style, char* text)
{
	return r_next_fit_ex(s, style, R_ICON_NONE, text);
}

rect r_next_fit_ex(r_layout_iter* s, int style, int icon, char* text)
{
	rect inner = r_measure_ex(s->l->context, style, icon, text);
	rect outer = r_outer(s->l->context, style, &inner);
	if(is_flow(s)) return flow_next(s, outer.w, outer.h);
	return r_next(s, stack_horizontal(s) ? outer.w : outer.h);
}

rect r_remaining(r_layout_iter* s)
{
	if(is_flow(s)) return flow_item(s, s->area.w, s->size);
	if(s->count > 0) r_cut_px(s->dir, &s->area, s->spacing, 0);
	rect r = s->area;
	r.id = stack_id(s, P800);
	s->count++;
	r_cut_px(s->dir, &s->area, stack_horizontal(s) ? s->area.w : s->area.h, 0);
	return r;
}

void r_gap(r_layout_iter* s, int size)
{
	if(is_flow(s)) { s->area.x += size; s->area.w -= size; return; }
	r_cut_px(s->dir, &s->area, size, 0);
}

r_layout_iter r_row(r_layout_iter* s, int size, r_alignment dir)
{
	return stack_nested(s, r_next(s, size), dir);
}

r_layout_iter r_group_begin(r_layout_iter* parent, int style)
{
	r_context* c = parent->l->context;
	rect r = parent->area;
	if(parent->count > 0) r_cut_px(parent->dir, &r, parent->spacing, 0);
	r.id = stack_id(parent, P810);
	if(!style) return stack_nested(parent, r, R_TOP);
	r_layout_iter group = r_layout(parent->l, r_inner(c, style, r), R_TOP, style);
	group.start.id = r.id;
	group.bg = r_style(parent->l, r, style);
	return group;
}

rect r_group_end(r_layout_iter* group, r_layout_iter* parent)
{
	int used = group->start.h - group->area.h;
	if(!group->bg && used == 0)
	{
		parent->count++;
		return (rect){0};
	}
	rect content = group->start;
	content.h = used;
	rect outer = group->bg ? r_outer(parent->l->context, group->bg->style, &content) : content;
	rect r = stack_horizontal(parent) ? r_next(parent, outer.w) : r_next(parent, outer.h);
	if(group->bg)
	{
		uint64_t id = group->bg->r.id;
		group->bg->r = r;
		group->bg->r.id = id;
	}
	return r;
}
