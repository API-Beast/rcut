#include "internal.h"

void __core_open_context_menu(r_interact* it, float list_x, float list_y);

static inline void xform_m23_inv(float m[2][3], float px, float py, float* ox, float* oy)
{
	float a = m[0][0], c = m[0][1], e = m[0][2];
	float b = m[1][0], d = m[1][1], f = m[1][2];
	float det = a * d - b * c;
	if(det == 0.0f) { *ox = px; *oy = py; return; }
	float inv = 1.0f / det;
	float tx = px - e, ty = py - f;
	*ox = (d * tx - c * ty) * inv;
	*oy = (a * ty - b * tx) * inv;
}

static inline void xform_from_target(r_ui_list* l, float tx, float ty, float* lx, float* ly)
{
	int ox, oy; rect clip;
	r_list_get_transform(l, &ox, &oy, &clip);
	float x = tx - ox, y = ty - oy;
	if(l->use_transform_matrix)
		xform_m23_inv(l->content_transform, x, y, lx, ly);
	else { *lx = x; *ly = y; }
}

static inline void xform_to_target(r_ui_list* l, float lx, float ly, float* tx, float* ty)
{
	int ox, oy; rect clip;
	r_list_get_transform(l, &ox, &oy, &clip);
	if(l->use_transform_matrix)
	{
		float px = l->content_transform[0][0] * lx + l->content_transform[0][1] * ly + l->content_transform[0][2];
		float py = l->content_transform[1][0] * lx + l->content_transform[1][1] * ly + l->content_transform[1][2];
		lx = px; ly = py;
	}
	*tx = ox + lx;
	*ty = oy + ly;
}

static inline bool list_matches_target(r_ui_list* l, uint64_t target)
{
	while(l->ephemeral_parent)
		l = l->ephemeral_parent;

	return l->target == target;
}

static bool is_blocked(r_context* c, r_ui_list* l);

static r_interact* list_find(r_ui_list* l, uint64_t id)
{
	if(!id) return NULL;
	int p = l->frame_parity;
	int* idx_ptr = hash_get(&l->id_hash[p], id);
	if(idx_ptr == NULL) return NULL;
	return &l->interact_items[*idx_ptr];
}

r_interact* find_impl(r_context* c, uint64_t id, bool skip_hidden)
{
	if(!id) return NULL;
	for(r_ui_list* l = c->chain_start; l != NULL; l = l->chain_next)
	{
		if(skip_hidden && !r_list_is_visible(l)) continue;
		r_interact* it = list_find(l, id);
		if(it) return it;
	}
	return NULL;
}

static inline r_interact* find(r_context* c, uint64_t id)        { return find_impl(c, id, false); }
static inline r_interact* find_visible(r_context* c, uint64_t id){ return find_impl(c, id, true);  }

static bool should_receive_mouse_event(r_context* c, r_interact* it)
{
	if(!c->pressed_id) return true; // No object is pressed, all interactables can receive mouse events.
	if(it->r.id == c->pressed_id) return true;
	if(c->pressed_group_id && it->press_group == c->pressed_group_id) return true;
	return false;
}

static r_interact* list_hit(r_context* c, r_ui_list* l, uint64_t target, float x, float y)
{
	if(!list_matches_target(l, target) || !r_list_is_visible(l) || l->is_tooltip || is_blocked(c, l)) return NULL;
	int ox, oy; rect clip;
	r_list_get_transform(l, &ox, &oy, &clip);
	if(!point_in(clip, x, y)) return NULL;
	float cx, cy; xform_from_target(l, x, y, &cx, &cy);
	r_interact* hit = NULL;
	for(int j = 0; j < l->count_interact_items; j++)
	{
		r_interact* it = &l->interact_items[j];
		if(!should_receive_mouse_event(c, it)) continue;
		if(point_in(r_dynamic_rect(it->dynamic, it->r), cx, cy)) hit = it;
	}
	return hit;
}

static r_interact* mouse_hit_test(r_context* c, uint64_t target, float x, float y)
{
	r_interact* hit = NULL;
	for(r_ui_list* l = c->chain_start; l != NULL; l = l->chain_next)
	{
		r_interact* h = list_hit(c, l, target, x, y);
		if(h) hit = h;
		else if(hit && l->block_event_fallthrough && l->is_ephemeral && r_list_is_visible(l) && list_matches_target(l, target))
			hit = NULL; // blocked by list (for example exclusive dialogs)
	}
	return hit;
}

static int mouse_hit_test_stack(r_context* c, uint64_t target, float x, float y, r_interact** out, int max)
{
	int n = 0;
	for(r_ui_list* l = c->chain_end; l != NULL && n < max; l = l->chain_prev)
	{
		if(!list_matches_target(l, target) || !r_list_is_visible(l)) continue;
		r_interact* hit = list_hit(c, l, target, x, y);
		if(hit) out[n++] = hit;
		if(l->block_event_fallthrough && (hit || l->is_ephemeral)) break;
	}
	return n;
}

static r_event make_event(r_context* c, r_interact* it)
{
	r_ui_list* l = it->owner;
	r_event e = {0};
	e.context = c; e.ui_list = l;
	e.mouse_state = c->mouse_buttons;
	e.modifier_state = c->modifier_flags;
	e.click_count = c->click_count;
	return e;
}

static r_event make_mouse_event(r_context* c, r_interact* it, float x, float y)
{
	r_ui_list* l = it->owner;
	r_event e = make_event(c, it);
	e.target_mouse_x = x; e.target_mouse_y = y;
	e.mouse_button = c->mouse_buttons;
	float cx, cy; xform_from_target(l, x, y, &cx, &cy);
	e.list_mouse_x = cx; e.list_mouse_y = cy;
	rect r = r_dynamic_rect(it->dynamic, it->r);
	e.rect_mouse_x = cx - r.x;
	e.rect_mouse_y = cy - r.y;
	xform_from_target(l, c->drag_origin_x, c->drag_origin_y, &e.drag_start_x, &e.drag_start_y);
	e.drag_delta_x = x - c->drag_origin_x;
	e.drag_delta_y = y - c->drag_origin_y;
	return e;
}

static r_event make_drag_event(r_context* c, r_interact* it, float x, float y)
{
	r_event e = make_mouse_event(c, it, x, y);
	e.mouse_button = c->drag_mouse_button;
	return e;
}

bool r_dispatch(r_interact* it, r_event_type t, r_event* e)
{
	if(it)
	for(r_callback* cb = it->callbacks; cb; cb = cb->next)
	if(cb->event == t && cb->function)
	if(cb->function(e, cb))
		return true;
	return false;
}

static bool dispatch_ex(r_interact* it, r_event_type t, r_event e)
{
	return r_dispatch(it, t, &e);
}

static inline bool has_callback(r_interact* it, r_event_type type)
{
	for(r_callback* cb = it->callbacks; cb; cb = cb->next)
	if(cb->event == type)
		return true;
	return false;
}

static bool is_first_part(r_ui_list* l, int index)
{
	return index == 0 || l->interact_items[index - 1].r.id != l->interact_items[index].r.id;
}

static bool widget_dispatch(r_interact* first, r_event_type t, r_event* e)
{
	if(first)
	for(r_interact* it = first; it->r.id == first->r.id; it++)
	if(r_dispatch(it, t, e))
		return true;

	return false;
}

static bool widget_dispatch_ex(r_interact* first, r_event_type t, r_event e)
{
	return widget_dispatch(first, t, &e);
}

static bool widget_has_callback(r_interact* first, r_event_type type)
{
	for(r_interact* it = first; it->r.id == first->r.id; it++)
	if(has_callback(it, type))
		return true;

	return false;
}

static r_interact* find_part(r_context* c, uint64_t id, int part)
{
	r_interact* first = find(c, id);
	if(!first || part <= 0) return first;
	for(int i = 1; i <= part; i++) if(first[i].r.id != first->r.id) return first;
	return first + part;
}

static inline bool is_focusable(r_interact* first)
{
	return widget_has_callback(first, R_ACTIVATION) || widget_has_callback(first, R_KEY_INPUT) || widget_has_callback(first, R_TEXT_INPUT);
}

static r_ui_list* modal_scope(r_context* c)
{
	r_ui_list* m = NULL;

	for(r_ui_list* l = c->chain_start; l; l = l->chain_next)
	if(((l->is_ephemeral && !l->is_viewport && l->block_event_fallthrough) || l->is_modal) && r_list_is_visible(l))
		m = l;

	return m;
}

static r_interact* focus_for(r_context* c, uint64_t target)
{
	r_interact* it = find_visible(c, c->focus_id);
	return it && list_matches_target(it->owner, target) && !is_blocked(c, it->owner) ? it : NULL;
}

static inline bool in_scope(r_ui_list* l, r_ui_list* scope)
{
	return !scope || l == scope || is_descendant(l, scope);
}

static bool is_blocked(r_context* c, r_ui_list* l)
{
	return !in_scope(l, modal_scope(c));
}

static void reconcile_focus(r_context* c)
{
	if(find_visible(c, c->focus_id)) return;
	r_interact* old_focus = find(c, c->focus_id);
	if(old_focus)
		widget_dispatch_ex(old_focus, R_FOCUS_EXIT, make_event(c, old_focus));

	// Unwind the focus stack to the most recent still-valid, visible element.
	while(c->focus_stack_end_idx != c->focus_stack_start_idx)
	{
		int top = (c->focus_stack_end_idx - 1) & 15;
		uint64_t id = c->focus_stack[top];

		r_interact* new_focus = find_visible(c, id);
		if(new_focus)
		{
			c->focus_id         = id;
			c->time_since_focus = 0;
			r_event e = make_event(c, new_focus);
			widget_dispatch(new_focus, R_FOCUS_ENTER, &e);
			return;
		}
		c->focus_stack_end_idx = top;
	}
	c->focus_id = 0;
}

static void update_hover(r_context* c)
{
	r_interact* hover = mouse_hit_test(c, c->mouse_target, c->mouse_x, c->mouse_y);
	uint64_t new_hover_id = hover ? hover->r.id : 0;
	r_interact* first = hover ? find(c, new_hover_id) : NULL;
	c->hover_part = first ? (int)(hover - first) : 0;
	if(new_hover_id != c->hover_id)
	{
		r_interact* old = find(c, c->hover_id);
		if(old)  { widget_dispatch_ex(old,   R_MOUSE_OUT, make_mouse_event(c, old,   c->mouse_x, c->mouse_y)); }
		if(first){ widget_dispatch_ex(first, R_MOUSE_IN,  make_mouse_event(c, first, c->mouse_x, c->mouse_y)); }
		c->hover_id = new_hover_id; c->time_since_hover = 0;
		c->hover_next_repeat = RCUT_MOUSE_HOVER_INITIAL_DELAY;
	}
}

static void reconcile(r_context* c)
{
	if(!find_visible(c, c->dragged_id))
	{
		r_interact* old_dragged = find_part(c, c->dragged_id, c->pressed_part);
		if(old_dragged)
			dispatch_ex(old_dragged, R_MOUSE_DRAG_RELEASE, make_drag_event(c, old_dragged, c->mouse_x, c->mouse_y));
		if(c->dragged_id) { c->pressed_id = 0; c->pressed_group_id = 0; }
		c->dragged_id = 0;
	}

	if(!find_visible(c, c->pressed_id))
	{
		c->pressed_id = 0;
		c->pressed_group_id = 0;
	}

	update_hover(c);
	reconcile_focus(c);
}

void r_list_reconcile(r_ui_list* l)
{
	if(l->context)
		reconcile(l->context);
}

uint64_t r_post_mouse_move(r_context* c, uint64_t target, float x, float y)
{
	c->mouse_x = x; c->mouse_y = y;
	c->mouse_target = target;

	// Active drag: stateful, only the dragged element.
	if(c->dragged_id)
	{
		r_interact* dragged = find_part(c, c->dragged_id, c->pressed_part);
		if(dragged)
			dispatch_ex(dragged, R_MOUSE_DRAG_MOVE, make_drag_event(c, dragged, x, y));
		return c->dragged_id;
	}

	// Captured by a pressed element.
	if(c->pressed_id)
	{
		// Promote to a drag once past the threshold (only the pressed element drags).
		r_interact* pressed = find_part(c, c->pressed_id, c->pressed_part);
		if(pressed)
		if(has_callback(pressed, R_MOUSE_DRAG_START) || has_callback(pressed, R_MOUSE_DRAG_MOVE) || has_callback(pressed, R_MOUSE_DRAG_RELEASE))
		{
			float dx = x - c->drag_origin_x, dy = y - c->drag_origin_y;
			float d = RCUT_MOUSE_DRAG_DISTANCE_DEFAULT;
			if(dx * dx + dy * dy >= d * d)
			{
				c->dragged_id = c->pressed_id;
				c->time_since_drag_start = 0;
				r_event e = make_drag_event(c, pressed, x, y);
				r_dispatch(pressed, R_MOUSE_DRAG_START, &e);
				r_dispatch(pressed, R_MOUSE_DRAG_MOVE, &e);
				return c->dragged_id;
			}
		}
	}
	uint64_t old_hover = c->hover_id;
	update_hover(c);
	return old_hover != c->hover_id ? c->hover_id : 0;
}

void r_advance_time(r_context* c, float dt)
{
	c->time_since_drag_start += dt;
	c->time_since_focus      += dt;
	c->time_since_hover      += dt;
	c->time_since_pressed    += dt;
	c->time_since_click      += dt;

	r_interact* pressed = find_part(c, c->pressed_id, c->pressed_part);
	r_interact* hover = find(c, c->hover_id);

	if(pressed)
	if(!c->dragged_id && c->hover_id == c->pressed_id)
	if(c->time_since_pressed >= c->pressed_next_repeat)
	{
		r_event e = make_mouse_event(c, pressed, c->mouse_x, c->mouse_y);
		e.mouse_button = c->pressed_mouse_button;
		r_dispatch(pressed, R_MOUSE_PRESSED, &e);
		c->pressed_next_repeat = c->time_since_pressed + RCUT_MOUSE_PRESSED_REPEAT;
	}

	if(hover)
	if(c->time_since_hover >= c->hover_next_repeat)
	{
		r_event e = make_mouse_event(c, hover, c->mouse_x, c->mouse_y);
		widget_dispatch(hover, R_MOUSE_HOVER, &e);
		c->hover_next_repeat = c->time_since_hover + RCUT_MOUSE_HOVER_REPEAT;
	}
}

bool r_close_popups(r_context* c, bool only_topmost)
{
	bool closed = false;
	for(r_ui_list* l = c->chain_end; l; l = l->chain_prev)
	{
		if(!l->close_on_outside_click || l->is_marked_as_to_delete || !r_list_is_visible(l)) continue;
		r_ephemeral_mark_to_delete(l->ephemeral_parent, l->root_area.id);
		closed = true;
		if(only_topmost) break;
	}
	return closed;
}

static void close_popups_outside(r_context* c, r_interact* hit)
{
	for(r_ui_list* l = c->chain_start; l; l = l->chain_next)
	{
		if(!l->close_on_outside_click || l->is_marked_as_to_delete || !r_list_is_visible(l)) continue;
		if(hit && (hit->owner == l || is_descendant(hit->owner, l) || hit->r.id == l->anchor_id)) continue;
		r_ephemeral_mark_to_delete(l->ephemeral_parent, l->root_area.id);
	}
}

uint64_t r_post_mouse_down(r_context* c, uint64_t target, int mouse_button, float x, float y)
{
	c->mouse_x = x; c->mouse_y = y;
	c->mouse_target = target;
	c->mouse_buttons |= (1 << mouse_button);
	c->is_in_navigation_mode = false;
	c->is_in_accelerator_mode = false;

	r_interact* stack[RCUT_HIT_STACK_MAX];
	int n = mouse_hit_test_stack(c, target, x, y, stack, RCUT_HIT_STACK_MAX);
	close_popups_outside(c, n ? stack[0] : NULL);
	if(n == 0)
	{
		r_change_focus(c, NULL);
		return 0;
	}

	r_interact* it = stack[0];

	float mdx = x - c->drag_origin_x, mdy = y - c->drag_origin_y;
	bool repeat = it->r.id == c->last_click_id && mouse_button == c->last_click_button && c->time_since_click < RCUT_MOUSE_MULTI_CLICK_TIME && mdx * mdx + mdy * mdy < 16.0f;
	c->click_count = repeat ? c->click_count + 1 : 1;
	c->last_click_id = it->r.id;
	c->last_click_button = mouse_button;
	c->time_since_click = 0;

	r_interact* first = find(c, it->r.id);
	c->pressed_id           = it->r.id;
	c->pressed_part         = first ? (int)(it - first) : 0;
	c->pressed_group_id     = it->press_group ? it->press_group : it->r.id;
	c->pressed_mouse_button = mouse_button;
	c->time_since_pressed   = 0;
	c->pressed_next_repeat  = RCUT_MOUSE_PRESSED_INITIAL_REPEAT;

	c->drag_origin_x = x; c->drag_origin_y = y; c->drag_mouse_button = mouse_button;
	if(first && is_focusable(first)) r_change_focus(c, first);

	for(int i = 0; i < n; i++)
	{
		r_event e = make_mouse_event(c, stack[i], x, y);
		e.mouse_button = mouse_button;
		if(r_dispatch(stack[i], R_MOUSE_PRESSED, &e)) break;
	}
	return it->r.id;
}

uint64_t r_post_mouse_up(r_context* c, uint64_t target, int mouse_button, float x, float y)
{
	c->mouse_x = x; c->mouse_y = y;
	c->mouse_target = target;
	c->mouse_buttons &= ~(1 << mouse_button);
	uint64_t ret = 0;
	r_interact* it;

	if(c->dragged_id && mouse_button == c->drag_mouse_button)
	{
		it = find_part(c, c->dragged_id, c->pressed_part);
		if(it)
		{
			r_event e = make_drag_event(c, it, x, y);
			r_dispatch(it, R_MOUSE_DRAG_MOVE, &e);
			r_dispatch(it, R_MOUSE_DRAG_RELEASE, &e);
			ret = it->r.id;
		}
		c->dragged_id = 0;
		c->pressed_id = 0; c->pressed_group_id = 0;
		update_hover(c);
		return ret;
	}

	it = c->pressed_id ? mouse_hit_test(c, target, x, y) : NULL;
	if(it)
	{
		r_event e = make_mouse_event(c, it, x, y);
		e.mouse_button = mouse_button;
		bool dispatched = false;
		if(mouse_button == R_MOUSE_LEFT)
			dispatched = r_dispatch(it, R_ACTIVATION, &e);
		if(!dispatched)
			dispatched = r_dispatch(it, R_MOUSE_CLICK, &e);
		r_interact* first = find(c, it->r.id);
		if(!dispatched && mouse_button == R_MOUSE_RIGHT && first && widget_has_callback(first, R_CONTEXT))
		{
			__core_open_context_menu(first, e.list_mouse_x, e.list_mouse_y);
			dispatched = true;
		}
		if(dispatched)
			ret = it->r.id;
	}

	// Release the capture when the capturing button is released.
	if(mouse_button == c->pressed_mouse_button)
	{
		c->pressed_id = 0;
		c->pressed_group_id = 0;
		update_hover(c);
	}
	return ret;
}

uint64_t r_post_text(r_context* c, uint64_t target, char* str)
{
	r_interact* it = focus_for(c, target);
	if(!it || !widget_has_callback(it, R_TEXT_INPUT))
	{
		it = find_visible(c, c->text_auto_focus_idx);
		if(it) r_change_focus(c, it);
	}
	if(it)
	{
		r_event e = make_event(c, it);
		e.text = str;
		if(widget_dispatch(it, R_TEXT_INPUT, &e))
			return it->r.id;
	}
	return 0;
}

uint64_t r_post_text_edit(r_context* c, uint64_t target, char* str, int start, int length)
{
	r_interact* it = focus_for(c, target);
	if(!it || !widget_has_callback(it, R_TEXT_EDIT))
	{
		it = find_visible(c, c->text_auto_focus_idx);
		if(it) r_change_focus(c, it);
	}
	if(it)
	{
		r_event e = make_event(c, it);
		e.text = str;
		e.ime_highlight_start = start;
		e.ime_highlight_length = length;
		if(widget_dispatch(it, R_TEXT_EDIT, &e))
			return it->r.id;
	}
	return 0;
}

void r_change_focus(r_context* c, r_interact* new_focus)
{
	uint64_t new_id = new_focus ? new_focus->r.id : 0;
	if(c->focus_id == new_id) return;

	r_interact* old_focus = find(c, c->focus_id);
	c->focus_id          = new_id;
	c->time_since_focus  = 0;
	if(new_focus) new_focus = find(c, new_id) ? find(c, new_id) : new_focus;
	if(old_focus)
		widget_dispatch_ex(old_focus, R_FOCUS_EXIT, make_event(c, old_focus));
	if(new_focus)
		widget_dispatch_ex(new_focus, R_FOCUS_ENTER, make_event(c, new_focus));

	if(new_id)
	{
		c->focus_stack[c->focus_stack_end_idx] = new_id;
		c->focus_stack_end_idx = (c->focus_stack_end_idx + 1) & 15;
		if(c->focus_stack_end_idx == c->focus_stack_start_idx)
			c->focus_stack_start_idx = (c->focus_stack_start_idx + 1) & 15;
	}
	else { c->focus_stack_start_idx = c->focus_stack_end_idx; }
}

static r_interact* find_default_next_focus(r_context* c, r_interact* it, bool reverse)
{
	r_ui_list* scope = modal_scope(c);
	r_interact *first = NULL, *last = NULL, *prev = NULL;
	bool found = false;
	for(r_ui_list* l = c->chain_start; l != NULL; l = l->chain_next)
	{
		if(!r_list_is_visible(l) || !in_scope(l, scope)) continue;
		for(int j = 0; j < l->count_interact_items; j++)
		{
			r_interact* cand = &l->interact_items[j];
			if(!is_first_part(l, j) || !is_focusable(cand)) continue;
			if(!first) first = cand;
			if(found && !reverse) return cand;
			if(it && cand->r.id == it->r.id)
			{
				found = true;
				if(reverse && prev) return prev;
			}
			prev = cand; last = cand;
		}
	}
	return reverse ? last : first;
}

static r_interact* find_directional_focus(r_context* c, r_interact* it, int dir)
{
	float cx_ls = it->r.x + it->r.w * 0.5f, cy_ls = it->r.y + it->r.h * 0.5f;
	float cx, cy;
	xform_to_target(it->owner, cx_ls, cy_ls, &cx, &cy);

	r_ui_list* scope = modal_scope(c);
	r_interact* best = NULL; float best_score = 1e30f;
	for(r_ui_list* l = c->chain_start; l != NULL; l = l->chain_next)
	{
		if(!r_list_is_visible(l) || !in_scope(l, scope)) continue;
		for(int j = 0; j < l->count_interact_items; j++)
		{
			r_interact* cand = &l->interact_items[j];
			if(cand->r.id == it->r.id || !is_first_part(l, j) || !is_focusable(cand)) continue;
			float clx = cand->r.x + cand->r.w * 0.5f, cly = cand->r.y + cand->r.h * 0.5f;
			float candcx, candcy; xform_to_target(l, clx, cly, &candcx, &candcy);
			float dx = candcx - cx, dy = candcy - cy;
			float primary, secondary;
			if     (dir == 0) { primary = -dy; secondary = dx < 0 ? -dx : dx; }
			else if(dir == 1) { primary = -dx; secondary = dy < 0 ? -dy : dy; }
			else if(dir == 2) { primary =  dy; secondary = dx < 0 ? -dx : dx; }
			else              { primary =  dx; secondary = dy < 0 ? -dy : dy; }
			if(primary <= 0.0f) continue;
			float score = primary + secondary * secondary / primary; // parabolic: same-axis proximity wins
			if(score < best_score) { best_score = score; best = cand; }
		}
	}
	return best;
}

uint64_t r_post_key_input(r_context* c, uint64_t target, int32_t keycode, int32_t scancode, uint32_t modifier_flags)
{
	r_interact* key_focus = focus_for(c, target);
	c->modifier_flags = modifier_flags;

	// Accelerator mode
	if(keycode == R_KEY_ALT)
	{
		bool blocked = key_focus && (key_focus->traps_alt_key || key_focus->owner->block_accelerator_mode);
		if(!blocked)
		{
			c->is_in_accelerator_mode = !c->is_in_accelerator_mode;
			if(c->is_in_accelerator_mode) c->is_in_navigation_mode = false;
			return key_focus ? key_focus->r.id : 0;
		}
	}
	if(c->is_in_accelerator_mode)
	{
		r_ui_list* scope = modal_scope(c);
		for(r_ui_list* l = c->chain_start; l != NULL; l = l->chain_next)
		{
			if(!r_list_is_visible(l) || !in_scope(l, scope)) continue;
			for(int j = 0; j < l->count_interact_items; j++)
			{
				r_interact* ai = &l->interact_items[j];
				if(ai->accelerator_key && ai->accelerator_key == keycode)
				{
					r_event e = make_event(c, ai);
					e.keycode = keycode; e.scancode = scancode; e.modifier_state = modifier_flags;
					r_dispatch(ai, R_ACTIVATION, &e);
					c->is_in_accelerator_mode = false;
					return ai->r.id;
				}
			}
		}
		c->is_in_accelerator_mode = false;
	}

	if(!key_focus)
	{
		if(keycode == R_KEY_TAB)
		{
			r_interact* next_focus = find_default_next_focus(c, NULL, (modifier_flags & R_MOD_SHIFT) != 0);
			c->is_in_navigation_mode = true;
			if(next_focus) { r_change_focus(c, next_focus); return next_focus->r.id; }
		}
		if(keycode == R_KEY_ESCAPE && r_close_popups(c, true)) return 1;
		return 0;
	}

	r_event e = make_event(c, key_focus);
	e.keycode = keycode; e.scancode = scancode; e.modifier_state = modifier_flags;

	// Hotkeys of context menu entries.
	for(r_interact* part = key_focus; part->r.id == key_focus->r.id; part++)
	for(r_callback* cb = part->callbacks; cb; cb = cb->next)
	if(cb->event == R_CONTEXT && cb->hotkey && cb->hotkey == (uint32_t)keycode && cb->hotkey_modifier == modifier_flags)
	if(cb->function && cb->function(&e, cb))
		return key_focus->r.id;

	bool wants_text = widget_has_callback(key_focus, R_TEXT_INPUT);
	if(keycode == R_KEY_ENTER || (keycode == R_KEY_SPACE && !wants_text))
	{
		if(widget_dispatch(key_focus, R_ACTIVATION, &e))
			return key_focus->r.id;
	}

	// Tab focus
	if(keycode == R_KEY_TAB && (!key_focus->traps_tab_focus || c->is_in_navigation_mode))
	{
		c->is_in_navigation_mode = true;
		bool shift = (modifier_flags & R_MOD_SHIFT) != 0;
		r_interact* next_focus = shift ? key_focus->previous_focus : key_focus->next_focus;
		if(!next_focus)
			next_focus = find_default_next_focus(c, key_focus, shift);

		if(next_focus)
		{
			r_change_focus(c, next_focus);
			return next_focus->r.id;
		}
		return key_focus->r.id;
	}

	// Arrow key navigation
	if(c->is_in_navigation_mode && !key_focus->traps_arrow_focus && (keycode == R_KEY_UP || keycode == R_KEY_LEFT || keycode == R_KEY_DOWN || keycode == R_KEY_RIGHT))
	{
		int dir = keycode == R_KEY_UP   ? 0 :
				  keycode == R_KEY_LEFT ? 1 :
				  keycode == R_KEY_DOWN ? 2 : 3;
		r_interact* next_focus = key_focus->directional_focus[dir];
		if(!next_focus)
			next_focus = find_directional_focus(c, key_focus, dir);

		if(next_focus)
		{
			r_change_focus(c, next_focus);
			return next_focus->r.id;
		}
		return key_focus->r.id;
	}

	if(widget_dispatch(key_focus, R_KEY_INPUT, &e))
		return key_focus->r.id;

	if(keycode == R_KEY_ESCAPE)
	{
		if(r_close_popups(c, true)) return key_focus->r.id;
		c->is_in_navigation_mode = false;
	}
	return 0;
}

uint64_t r_post_mouse_wheel(r_context* c, uint64_t target, float x, float y, float scroll_x, float scroll_y)
{
	c->mouse_x = x; c->mouse_y = y;
	c->mouse_target = target;
	r_interact* stack[RCUT_HIT_STACK_MAX];
	int n = mouse_hit_test_stack(c, target, x, y, stack, RCUT_HIT_STACK_MAX);
	for(int i = 0; i < n; i++)
	{
		r_event e = make_mouse_event(c, stack[i], x, y);
		e.scroll_x = scroll_x; e.scroll_y = scroll_y;
		if(r_dispatch(stack[i], R_MOUSE_SCROLL, &e)) return stack[i]->r.id;
	}
	return 0;
}

bool r_is_pressed_visual(r_context* c, uint64_t id)
{
	return id && id == c->pressed_id && c->hover_id == c->pressed_id;
}

bool r_focus_wants_text(r_context* c)
{
	r_interact* it = find_visible(c, c->focus_id);
	return it && widget_has_callback(it, R_TEXT_INPUT);
}

void r_damage_clear(r_context* c)
{
	for(r_ui_list* l = c->chain_start; l; l = l->chain_next)
		l->damage = (rect){0};
}

static const char* event_name(int e)
{
    switch(e)
    {
        case R_ACTIVATION:         return "ACTIVATION";
        case R_CONTEXT:            return "CONTEXT";
        case R_TEXT_INPUT:         return "TEXT_INPUT";
        case R_TEXT_EDIT:          return "TEXT_EDIT";
        case R_KEY_INPUT:          return "KEY_INPUT";
        case R_MOUSE_IN:           return "MOUSE_IN";
        case R_MOUSE_OUT:          return "MOUSE_OUT";
		case R_MOUSE_HOVER:        return "MOUSE_HOVER";
        case R_MOUSE_CLICK:        return "MOUSE_CLICK";
        case R_MOUSE_PRESSED:      return "MOUSE_PRESSED";
        case R_MOUSE_DRAG_START:   return "MOUSE_DRAG_START";
        case R_MOUSE_DRAG_MOVE:    return "MOUSE_DRAG_MOVE";
        case R_MOUSE_DRAG_RELEASE: return "MOUSE_DRAG_RELEASE";
        case R_MOUSE_SCROLL:       return "MOUSE_SCROLL";
        case R_FOCUS_ENTER:        return "FOCUS_ENTER";
        case R_FOCUS_EXIT:         return "FOCUS_EXIT";
        default:                   return "???UNKNOWN???";
    }
}

void r_list_print(r_ui_list* l)
{
    if(!l) { printf("(null ui_list)\n"); return; }

    printf("r_ui_list %p  target=%llu  parent=%p\n",
           (void*)l,
           (unsigned long long)l->target,
           (void*)l->ephemeral_parent);
    printf("  root_area: x=%d y=%d w=%d h=%d (id=%llu)\n",
           l->root_area.x, l->root_area.y, l->root_area.w, l->root_area.h,
           (unsigned long long)l->root_area.id);
    printf("  flags: masked=%d hidden=%d block_fallthrough=%d block_accel=%d frame_parity=%d\n",
           l->is_masked, l->is_hidden, l->block_event_fallthrough,
           l->block_accelerator_mode, l->frame_parity);

    // Drawables
    printf("  drawables (%d):\n", l->count_draw_items);
    for(int i = 0; i < l->count_draw_items; i++)
    {
        r_drawable* d = &l->draw_items[i];
        printf("    [%3d] %-8X x=%-5d y=%-5d w=%-5d h=%-5d id=%-20llX",
					i, d->style, d->r.x, d->r.y, d->r.w, d->r.h, (unsigned long long)d->r.id);
        if(d->text) printf("  \"%s\"", d->text);
        if(d->dynamic) printf("  [dynamic %d]", d->dynamic->kind);
        printf("\n");
    }

    // Interactables
    printf("  interactables (%d):\n", l->count_interact_items);
    for(int i = 0; i < l->count_interact_items; i++)
    {
        r_interact* it = &l->interact_items[i];
        printf("    [%3d] x=%-5d y=%-5d w=%-5d h=%-5d id=%-20llX",
               i, it->r.x, it->r.y, it->r.w, it->r.h,
               (unsigned long long)it->r.id);
        if(it->traps_tab_focus)   printf(" [tab]");
        if(it->traps_arrow_focus) printf(" [arrow]");
        if(it->traps_alt_key)     printf(" [alt]");
        if(it->accelerator_key)   printf(" [accel=%d]", it->accelerator_key);
        if(it->press_group)       printf(" [group=%llu]", (unsigned long long)it->press_group);
        printf("\n");

        // Callback list
        for(r_callback* cb = it->callbacks; cb; cb = cb->next)
        {
            printf("            -> %s%s",
                   event_name(cb->event),
                   cb->function ? "" : " (no fn)");
            if(cb->menu_label) printf("  menu=\"%s\"", cb->menu_label);
            if(cb->hotkey)     printf("  hotkey=0x%x", cb->hotkey);
            if(cb->size)       printf("  [data %d bytes]", cb->size);
            printf("\n");
        }
    }
}
r_interact* r_find_interact(r_context* c, uint64_t id)
{
	return find_visible(c, id);
}

r_cursor r_hover_cursor(r_context* c)
{
	r_interact* it = c->dragged_id ? find_part(c, c->dragged_id, c->pressed_part) : find_part(c, c->hover_id, c->hover_part);
	return it && r_list_is_visible(it->owner) ? it->cursor : R_CURSOR_DEFAULT;
}
