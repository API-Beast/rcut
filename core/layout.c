#include "internal.h"

rect r_cut_px(r_alignment dir, rect* target, int pixels, uint64_t salt)
{
	rect result = {0};
	rect old    = *target;
	update_id(&result.id, target->id, P50KK, P400, salt);
	if(!(dir & (R_BOTTOM | R_TOP | R_RIGHT | R_LEFT)))
	{
		assert(false && "unknown direction");
		dir = R_TOP;
	}
	if(dir & R_BOTTOM)
	{
		target->h = MAX(target->h - pixels, 0);
		result.h = old.h - target->h;
		result.w = old.w;
		result.x = target->x;
		result.y = target->y + target->h;
		return result;
	}
	else if(dir & R_TOP)
	{
		target->h  = MAX(target->h - pixels, 0);
		result.h   = old.h - target->h;
		target->y += result.h;
		result.w = old.w;
		result.x = target->x;
		result.y = old.y;
		return result;
	}
	else if(dir & R_RIGHT)
	{
		target->w = MAX(target->w - pixels, 0);
		result.w = old.w - target->w;
		result.h = old.h;
		result.y = target->y;
		result.x = target->x + target->w;
		return result;
	}
	else
	{
		target->w  = MAX(target->w - pixels, 0);
		result.w   = old.w - target->w;
		target->x += result.w;
		result.h = old.h;
		result.y = target->y;
		result.x = old.x;
		return result;
	}
}

rect r_expand_px(r_alignment dir, rect* target, int pixels, uint64_t salt)
{
	rect result = {0};
	rect old    = *target;
	update_id(&result.id, target->id, P50KK, P410, salt);
	if(!(dir & (R_BOTTOM | R_TOP | R_RIGHT | R_LEFT)))
	{
		assert(false && "unknown direction");
		dir = R_TOP;
	}
	if(dir & R_BOTTOM)
	{
		result.w   = old.w;
		result.h   = pixels;
		result.x   = old.x;
		result.y   = old.y + old.h;
		target->h += pixels;
		return result;
	}
	else if(dir & R_TOP)
	{
		result.w   = old.w;
		result.h   = pixels;
		result.x   = old.x;
		result.y   = old.y - pixels;
		target->y -= pixels;
		target->h += pixels;
		return result;
	}
	else if(dir & R_RIGHT)
	{
		result.h   = old.h;
		result.w   = pixels;
		result.x   = old.x + old.w;
		result.y   = old.y;
		target->w += pixels;
		return result;
	}
	else
	{
		result.h   = old.h;
		result.w   = pixels;
		result.x   = old.x - pixels;
		result.y   = old.y;
		target->x -= pixels;
		target->w += pixels;
		return result;
	}
}

rect r_offset(rect r, int x_offset, int y_offset)
{
	return (rect){.x = r.x + x_offset, .y = r.y + y_offset, .w = r.w, .h = r.h, .id = r.id};
}

rect r_grow(rect r, int pixels)
{
	return (rect){.x = r.x - pixels, .y = r.y - pixels, .w = r.w + pixels*2, .h = r.h + pixels*2, .id = r.id};
}

rect r_grow_sides(rect r, int left, int top, int right, int bottom)
{
	return (rect){.x = r.x - left, .y = r.y - top, .w = r.w + left + right, .h = r.h + top + bottom, .id = r.id};
}

void r_align_fraction(rect target, rect* r, float x_fraction, float y_fraction)
{
	update_id(&r->id, r->id, target.id, P40K, P650);
	r->x = (int)(target.x + (target.w - r->w) * x_fraction);
	r->y = (int)(target.y + (target.h - r->h) * y_fraction);
}

void r_align(r_alignment corner, rect target, rect* r)
{
	float xf = 0.5f, yf = 0.5f;
	if(corner & R_LEFT)   xf = 0.0f;
	if(corner & R_RIGHT)  xf = 1.0f;
	if(corner & R_TOP)    yf = 0.0f;
	if(corner & R_BOTTOM) yf = 1.0f;
	r_align_fraction(target, r, xf, yf);
}

void r_align_group(r_alignment corner, rect target, rect** rects, int num_rects)
{
	rect before = *rects[0];
	r_align(corner, target, rects[0]);
	int x_delta = rects[0]->x - before.x;
	int y_delta = rects[0]->y - before.y;
	for(int i = 1; i < num_rects; i++)
	{
		rects[i]->x += x_delta;
		rects[i]->y += y_delta;
	}
}

rect r_aligned_fraction(rect target, rect src, float x_fraction, float y_fraction)
{
	return (rect){
		.x = (int)(target.x + (target.w - src.w) * x_fraction),
		.y = (int)(target.y + (target.h - src.h) * y_fraction),
		.w = src.w,
		.h = src.h,
		.id = xxhash64(src.id, target.id, P40K, P650)
	};
}

rect r_aligned(r_alignment corner, rect target, rect src)
{
	float xf = 0.5f, yf = 0.5f;
	if(corner & R_LEFT)   xf = 0.0f;
	if(corner & R_RIGHT)  xf = 1.0f;
	if(corner & R_TOP)    yf = 0.0f;
	if(corner & R_BOTTOM) yf = 1.0f;
	return r_aligned_fraction(target, src, xf, yf);
}

void r_aside_fraction(rect target, rect* r, float x_fraction, float y_fraction)
{
	update_id(&r->id, r->id, target.id, P80K, P560);
	if(fabsf(x_fraction - 0.5f) > fabsf(y_fraction - 0.5f))
		x_fraction = x_fraction < 0.5f ? 0.0f : 1.0f;
	else
		y_fraction = y_fraction < 0.5f ? 0.0f : 1.0f;

	r->x = (int)(target.x + (target.w + r->w) * x_fraction - r->w);
	r->y = (int)(target.y + (target.h + r->h) * y_fraction - r->h);
}

void r_aside(r_alignment corner, rect target, rect* r)
{
	float xf = 0.5f, yf = 0.5f;
	if(corner & R_LEFT)   xf = 0.0f;
	if(corner & R_RIGHT)  xf = 1.0f;
	if(corner & R_TOP)    yf = 0.0f;
	if(corner & R_BOTTOM) yf = 1.0f;
	r_aside_fraction(target, r, xf, yf);
}

void r_aside_group(r_alignment corner, rect target, rect** rects, int num_rects)
{
	rect before = *rects[0];
	r_aside(corner, target, rects[0]);
	int x_delta = rects[0]->x - before.x;
	int y_delta = rects[0]->y - before.y;
	for(int i = 1; i < num_rects; i++)
	{
		rects[i]->x += x_delta;
		rects[i]->y += y_delta;
	}
}


void r_relative_to_fraction(rect target, rect* r, float x_fraction, float y_fraction)
{
	update_id(&r->id, r->id, target.id, P40K, P750);
	r->x += (int)(target.x + target.w * x_fraction);
	r->y += (int)(target.y + target.h * y_fraction);
}

void r_relative_to(r_alignment corner, rect target, rect* r)
{
	float xf = 0.5f, yf = 0.5f;
	if(corner & R_LEFT)   xf = 0.0f;
	if(corner & R_RIGHT)  xf = 1.0f;
	if(corner & R_TOP)    yf = 0.0f;
	if(corner & R_BOTTOM) yf = 1.0f;
	r_relative_to_fraction(target, r, xf, yf);
}

rect rect_expand_pt(rect a, int x, int y)
{
	if(x < a.x){ a.w += (a.x - x); a.x = x; }
	else if(x > (a.x+a.w)){ a.w = (x - a.x);}
	if(y < a.y){ a.h += (a.y - y); a.y = y; }
	else if(y > (a.y+a.h)){ a.h = (y - a.y);}
	return a;
}

rect rect_expand(rect a, rect b)
{
	return rect_expand_pt(rect_expand_pt(a, b.x, b.y), b.x+b.w, b.y+b.h);
}

rect r_salt(rect r, uint64_t salt)
{
	update_id(&r.id, r.id, salt, 0, 0);
	return r;
}

void r_fill(r_alignment dir, rect target, rect** r, int num_rects)
{
	if(num_rects == 0) return;

	bool horizontal = (dir & (R_LEFT | R_RIGHT | R_H_CENTER)) != 0;
	bool reverse    = horizontal ? dir & R_RIGHT : dir & R_BOTTOM;

	int needed_px = 0;
	for(int i = 0; i < num_rects; i++)
		needed_px += horizontal ? (*r[i]).w : (*r[i]).h;

	int extra_px = (horizontal ? target.w : target.h) - needed_px;
	int per = extra_px / num_rects;
	int rem = extra_px - per * num_rects;
	int pos = horizontal ? target.x : target.y;
	int start = 0, end = num_rects, step = 1;
	if(reverse)
	{
		pos = horizontal ? target.x + target.w : target.y + target.h;
		start = num_rects - 1; end   = -1; step  = -1;
	}

	for(int i = start; i != end; i += step)
	{
		update_id(&(*r[i]).id, (*r[i]).id, target.id, P40K, (uint64_t)i + P600);
		int add = per + (i < rem ? 1 : 0);
		if(horizontal)
		{
			(*r[i]).w += add;
			if(reverse) pos -= (*r[i]).w;
			(*r[i]).x = pos; (*r[i]).y = target.y; (*r[i]).h = target.h;
			if(!reverse) pos += (*r[i]).w;
		}
		else
		{
			(*r[i]).h += add;
			if(reverse) pos -= (*r[i]).h;
			(*r[i]).y = pos; (*r[i]).x = target.x; (*r[i]).w = target.w;
			if(!reverse) pos += (*r[i]).h;
		}
	}
}

void r_min_size(rect *r, int w, int h)
{
	if(r->w < w) r->w = w;
	if(r->h < h) r->h = h;
}

rect r_inner(r_context* c, int style, rect outer)
{
	rect result = outer;
	if(c->interface.calc_inner_rect)
		result = c->interface.calc_inner_rect(c, style, outer);
	else
		result = r_grow(result, -2);
	update_id(&result.id, outer.id, P20K, P680, style);
	return result;
}

rect r_outer(r_context* c, int style, rect* inner)
{
	rect result = *inner;
	if(c->interface.calc_outer_rect)
		result = c->interface.calc_outer_rect(c, style, inner);
	else
		result = r_grow(result, 2);
	update_id(&result.id, inner->id, P20K, P600, style);
	return result;
}

rect rect_intersect(rect a, rect b)
{
	int x0 = MAX(a.x, b.x), y0 = MAX(a.y, b.y);
	int x1 = MIN(a.x + a.w, b.x + b.w), y1 = MIN(a.y + a.h, b.y + b.h);
	return (rect){.x = x0, .y = y0, .w = MAX(x1 - x0, 0), .h = MAX(y1 - y0, 0), .id = a.id};
}

void r_cut_group(r_alignment dir, rect* to_cut, rect** r, int num_rects)
{
	bool horizontal = (dir & (R_LEFT | R_RIGHT)) != 0;
	for(int i = 0; i < num_rects; i++)
	{
		rect cut = r_cut_px(dir, to_cut, horizontal ? r[i]->w : r[i]->h, i);
		cut.id = xxhash64(r[i]->id, cut.id, P40K, P700);
		*r[i] = cut;
	}
}

void r_expand_group(r_alignment dir, rect* to_expand, rect** r, int num_rects)
{
	bool horizontal = (dir & (R_LEFT | R_RIGHT)) != 0;
	for(int i = 0; i < num_rects; i++)
	{
		rect added = r_expand_px(dir, to_expand, horizontal ? r[i]->w : r[i]->h, i);
		added.id = xxhash64(r[i]->id, added.id, P40K, P710);
		*r[i] = added;
	}
}

rect r_remove_margins(r_context* c, int style, rect with_margins)
{
	int m = r_layout_default_spacing(c, style) / 2;
	rect result = r_grow(with_margins, -m);
	update_id(&result.id, with_margins.id, P40K, P640, style);
	return result;
}

rect r_with_margins(r_context* c, int style, rect outer)
{
	int m = r_layout_default_spacing(c, style) / 2;
	rect result = r_grow(outer, m);
	update_id(&result.id, outer.id, P80K, P680, style);
	return result;
}

rect r_measure_n(r_context* c, int style, const char* text, int len)
{
	if(!text) return (rect){0};
	if(c->interface.measure_text)
		return c->interface.measure_text(c, style, text, len);
	if(len < 0) len = (int)strlen(text);
	int lines = 1, w = 0, max_w = 0;
	for(int i = 0; i < len; i++)
	{
		if(text[i] == '\n') { lines++; w = 0; continue; }
		if((text[i] & 0xC0) != 0x80) w += 8;
		max_w = MAX(max_w, w);
	}
	return (rect){.w = max_w, .h = lines * 16};
}

rect r_measure(r_context* c, int style, const char* label)
{
	return r_measure_n(c, style, label, -1);
}

rect r_measure_ex(r_context* c, int style, int icon, const char* text)
{
	rect t = r_measure_n(c, style, text, -1);
	if(!icon) return t;
	int gap = 4;
	rect i = c->interface.measure_icon ? c->interface.measure_icon(c, style, icon, &gap) : (rect){.w = 16, .h = 16};
	if(!text) return i;
	return (rect){.w = i.w + gap + t.w, .h = MAX(i.h, t.h)};
}

int break_text(r_context* c, int style, const char* text, int len, int width, bool force, int* w)
{
	if(c->interface.break_text) return c->interface.break_text(c, style, text, len, width, force, w);
	int i = 0, x = 0, end = 0;
	*w = 0;
	while(i < len && text[i] != '\n')
	{
		while(i < len && text[i] != ' ' && text[i] != '\n') x += (text[i++] & 0xC0) != 0x80 ? 8 : 0;
		if(x > width && (end > 0 || !force)) return end;
		*w = x;
		while(i < len && text[i] == ' ') { x += 8; i++; }
		end = i;
	}
	*w = x;
	return end;
}
