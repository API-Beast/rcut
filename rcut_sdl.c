#include "rcut_sdl.h"
#include <stddef.h>

static void     __sdl_frontend_set_clipboard      (r_context* ctx, const char* text);
static char*    __sdl_frontend_get_clipboard      (r_context* ctx);
static uint64_t __sdl_frontend_get_viewport_target(r_ui_list* parent, rect r);

r_sdl_frontend* r_sdl_init(const char* title, int w, int h, int flags)
{
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) return NULL;
    
    SDL_WindowFlags sdl_flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if(flags & R_SDL_RESIZABLE) sdl_flags |= SDL_WINDOW_RESIZABLE;

    SDL_Window* root_window = SDL_CreateWindow(title, w, h, sdl_flags);
    if(!root_window) { SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS); return NULL; }
    r_sdl_frontend* c = calloc(1, sizeof(r_sdl_frontend));
    c->windows[0].window = root_window;

    c->auto_scale = true;
    c->vsync = flags & R_SDL_VSYNC;
    float scale = SDL_GetWindowDisplayScale(root_window);
    if(scale <= 0.0f) scale = 1.0f;

    r_init_context(&c->context);
    r_draw_software_init(&c->software, 0, 0, RCUT_SDL_CACHE_PIXELS);
    r_draw_backend backend = r_draw_software_backend(&c->software);
    r_draw_init(&c->draw, &c->context, &backend, scale);
    c->context.interface.set_clipboard       = __sdl_frontend_set_clipboard;
    c->context.interface.get_clipboard       = __sdl_frontend_get_clipboard;
    c->context.interface.get_viewport_target = __sdl_frontend_get_viewport_target;

    SDL_GetWindowSizeInPixels(root_window, &w, &h);
    c->root = r_create_ui_list(&c->context, 0, 0, (int)(w / scale), (int)(h / scale));
    c->last_tick_time = SDL_GetTicksNS();

    r_draw_find_system_fonts(&c->draw, true);
    r_theme_default(&c->draw, NULL);
    return c;
}

void r_sdl_update(r_sdl_frontend* c)
{
    uint64_t cur_time = SDL_GetTicksNS();
    r_advance_time(&c->context, (cur_time - c->last_tick_time) / 1.0e+9 );
    c->last_tick_time = cur_time;
    r_context_sweep(&c->context);

    r_cursor cursor = r_hover_cursor(&c->context);
    if(cursor != c->cursor)
    {
        static const SDL_SystemCursor map[] = { SDL_SYSTEM_CURSOR_DEFAULT, SDL_SYSTEM_CURSOR_TEXT, SDL_SYSTEM_CURSOR_POINTER, SDL_SYSTEM_CURSOR_EW_RESIZE, SDL_SYSTEM_CURSOR_NS_RESIZE };
        if(!c->cursors[cursor]) c->cursors[cursor] = SDL_CreateSystemCursor(map[cursor]);
        if(c->cursors[cursor]) SDL_SetCursor(c->cursors[cursor]);
        c->cursor = cursor;
    }

    SDL_Window* text_window = NULL;
    r_interact* it = r_focus_wants_text(&c->context) ? r_find_interact(&c->context, c->context.focus_id) : NULL;
    if(it)
    {
        r_ui_list* root = it->owner;
        while(root->ephemeral_parent) root = root->ephemeral_parent;
        text_window = root->target < RCUT_SDL_MAX_WINDOWS && c->windows[root->target].window ? c->windows[root->target].window : c->windows[0].window;
    }
    if(text_window != c->text_input_window)
    {
        if(c->text_input_window) SDL_StopTextInput(c->text_input_window);
        if(text_window) SDL_StartTextInput(text_window);
        c->text_input_window = text_window;
    }
}

static void close_window(r_sdl_frontend* c, int i)
{
    r_sdl_window* w = &c->windows[i];
    if(!w->window) return;
    for(int j = 1; j < RCUT_SDL_MAX_WINDOWS; j++)
        if(j != i && c->windows[j].parent == i) close_window(c, j);
    SDL_DestroySurface(w->internal_surface);
    SDL_DestroyWindow(w->window);
    w->internal_surface = NULL;
    w->window = NULL;
    w->last_window_surface = NULL;
}

void r_sdl_free(r_sdl_frontend* c)
{
    r_draw_free(&c->draw);
    r_free_context(&c->context);
    r_draw_software_free(&c->software);
    SDL_free(c->clipboard);
    for(int i = 0; i < 8; i++)
        if(c->cursors[i]) SDL_DestroyCursor(c->cursors[i]);
    close_window(c, 0);
    free(c);
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
}

static int window_index(r_sdl_frontend* c, SDL_WindowID id)
{
    for(int i = 0; i < RCUT_SDL_MAX_WINDOWS; i++)
        if(c->windows[i].window && SDL_GetWindowID(c->windows[i].window) == id)
            return i;
    return -1;
}

bool r_sdl_quit_requested(r_sdl_frontend* c, bool clear)
{
    bool result = c->quit_requested;
    if(clear) c->quit_requested = false;
    return result;
}

r_ui_list* r_sdl_dialog(r_sdl_frontend* c, uint64_t root_id, int w, int h, const char* title)
{
    int i = 1;
    while(i < RCUT_SDL_MAX_WINDOWS && c->windows[i].used) i++;
    if(i < RCUT_SDL_MAX_WINDOWS) close_window(c, i);
    if(i < RCUT_SDL_MAX_WINDOWS) c->windows[i] = (r_sdl_window){ .used = true, .domain = i };
    else i = 0;
    r_ui_list* l = r_create_ui_list(&c->context, (uint64_t)i, root_id, w, h);
    if(i == 0) r_align(R_CENTER, c->root->root_area, &l->root_area);
    l->root_area.id = root_id;
    l->is_modal = true;
    l->title = (char*)title;
    return l;
}

bool r_sdl_close_requested(r_sdl_frontend* c, r_ui_list* l, bool clear)
{
    if(l->target == 0 || l->target >= RCUT_SDL_MAX_WINDOWS) return false;
    bool result = c->windows[l->target].close_requested;
    if(clear) c->windows[l->target].close_requested = false;
    return result;
}

static void update_root_sizes(r_sdl_frontend* c)
{
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(c->windows[0].window, &w, &h);
    
    for(r_ui_list* l = c->context.chain_start; l != NULL; l = l->chain_next)
    if(l->ephemeral_parent == NULL && l->target == 0)
    {
        int lw = (int)(w / c->draw.scale), lh = (int)(h / c->draw.scale);
        if(lw != l->root_area.w || lh != l->root_area.h) l->resized = true;
        l->root_area.w = lw;
        l->root_area.h = lh;
        l->document_area = l->root_area;
    }
}

void r_sdl_set_scale(r_sdl_frontend* c, float scale)
{
    c->auto_scale = scale <= 0.0f;
    if(c->auto_scale) scale = SDL_GetWindowDisplayScale(c->windows[0].window);
    if(scale <= 0.0f) scale = 1.0f;
    if(scale == c->draw.scale) return;
    r_draw_set_scale(&c->draw, scale);
    update_root_sizes(c);
}

static void __sdl_frontend_set_clipboard(r_context* ctx, const char* text)
{
    SDL_SetClipboardText(text);
}

static char* __sdl_frontend_get_clipboard(r_context* ctx)
{
    r_sdl_frontend* c = (r_sdl_frontend*)((char*)ctx->userptr - offsetof(r_sdl_frontend, draw));
    SDL_free(c->clipboard);
    c->clipboard = SDL_GetClipboardText();
    return c->clipboard;
}

static SDL_Window* create_window(r_sdl_frontend* c, SDL_PropertiesID props)
{
    SDL_Window* window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    return window;
}

static uint64_t __sdl_frontend_get_viewport_target(r_ui_list* parent, rect r)
{
    r_sdl_frontend* c = (r_sdl_frontend*)((char*)parent->context->userptr - offsetof(r_sdl_frontend, draw));
    if(parent->target >= RCUT_SDL_MAX_WINDOWS || c->no_popup_windows) return parent->target;
    for(int i = 1; i < RCUT_SDL_MAX_WINDOWS; i++)
        if(!c->windows[i].used)
        {
            r_sdl_window* w = &c->windows[i];
            if(w->parent != (int)parent->target) close_window(c, i);
            w->used = true; w->parent = (int)parent->target; w->domain = 0; w->last_window_surface = NULL;
            return (uint64_t)i;
        }
    return parent->target;
}

static void draw_into_domain(r_sdl_frontend* c, int i)
{
    for(r_ui_list* l = c->context.chain_start; l; l = l->chain_next)
        if(l->target == (uint64_t)i)
        {
            l->target = (uint64_t)c->windows[i].domain;
            if(!l->ephemeral_parent)
            {
                uint64_t id = l->root_area.id;
                r_align(R_CENTER, c->root->root_area, &l->root_area);
                l->root_area.id = id;
            }
        }
    close_window(c, i);
    c->windows[i].used = false;
}

static void update_dialog_window(r_sdl_frontend* c, r_ui_list* l, int i)
{
    r_sdl_window* w = &c->windows[i], *p = &c->windows[w->parent];
    float s = c->draw.scale / SDL_GetWindowPixelDensity(c->windows[0].window);
    int ww = (int)SDL_ceilf(l->root_area.w * s), wh = (int)SDL_ceilf(l->root_area.h * s);
    if(ww != w->w || wh != w->h) close_window(c, i);
    if(!w->window)
    {
        int px = 0, py = 0, pw = 0, ph = 0;
        SDL_GetWindowPosition(p->window, &px, &py);
        SDL_GetWindowSize(p->window, &pw, &ph);
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_PARENT_POINTER, p->window);
        SDL_SetStringProperty (props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, l->title ? l->title : "");
        SDL_SetNumberProperty (props, SDL_PROP_WINDOW_CREATE_X_NUMBER, px + (pw - ww) / 2);
        SDL_SetNumberProperty (props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, py + (ph - wh) / 2);
        SDL_SetNumberProperty (props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, ww);
        SDL_SetNumberProperty (props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, wh);
        SDL_SetNumberProperty (props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, SDL_WINDOW_MODAL | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        w->window = create_window(c, props);
        if(!w->window) { w->domain = 0; draw_into_domain(c, i); return; }
    }
    else if(l->title && SDL_strcmp(SDL_GetWindowTitle(w->window), l->title) != 0) SDL_SetWindowTitle(w->window, l->title);
    w->w = ww; w->h = wh;
}

static void take_spare(r_sdl_frontend* c, int i, bool tooltip)
{
    r_sdl_window* w = &c->windows[i];
    for(int j = 1; j < RCUT_SDL_MAX_WINDOWS; j++)
    {
        r_sdl_window* s = &c->windows[j];
        if(j == i || s->used || !s->window || s->parent != w->parent || s->tooltip != tooltip) continue;
        for(int k = 1; k < RCUT_SDL_MAX_WINDOWS; k++)
            if(k != j && c->windows[k].parent == j) close_window(c, k);
        r_sdl_window keep = *w;
        *w = *s;
        w->used = true; w->parent = keep.parent; w->domain = keep.domain; w->last_window_surface = NULL;
        *s = (r_sdl_window){0};
        return;
    }
}

static void update_windows(r_sdl_frontend* c)
{
    uint8_t state[RCUT_SDL_MAX_WINDOWS] = {0};
    float s = c->draw.scale, density = SDL_GetWindowPixelDensity(c->windows[0].window);
    for(r_ui_list* l = c->context.chain_start; l; l = l->chain_next)
        if(l->target > 0 && l->target < RCUT_SDL_MAX_WINDOWS) state[l->target] = 1;
    for(int i = 1; i < RCUT_SDL_MAX_WINDOWS; i++)
        if(!state[i]) c->windows[i].used = false;

    for(r_ui_list* l = c->context.chain_start; l; l = l->chain_next)
    {
        uint64_t i = l->target;
        if(i == 0 || i >= RCUT_SDL_MAX_WINDOWS || !c->windows[i].used) continue;
        r_sdl_window* w = &c->windows[i], *p = &c->windows[w->parent];
        if(!l->ephemeral_parent)
        {
            w->domain = (int)i;
            w->x = w->y = 0;
            if(l->is_hidden || l->root_area.w <= 0 || l->root_area.h <= 0 || !p->window) continue;
            state[i] = 2;
            update_dialog_window(c, l, (int)i);
            continue;
        }
        if(l->ephemeral_parent->target == i) continue;
        w->domain = p->domain;
        if(c->no_popup_windows) { draw_into_domain(c, (int)i); continue; }
        rect b = r_draw_target_bounds(&c->draw, i);
        if(b.w <= 0 || b.h <= 0 || !p->window) continue;
        state[i] = 2;

        int x = (int)SDL_floorf((b.x * s - p->x) / density), y = (int)SDL_floorf((b.y * s - p->y) / density);
        int ww = (int)SDL_ceilf(((b.x + b.w) * s - p->x) / density) - x, wh = (int)SDL_ceilf(((b.y + b.h) * s - p->y) / density) - y;
        int px = p->x + (int)SDL_roundf(x * density), py = p->y + (int)SDL_roundf(y * density);
        if(w->window && w->tooltip != l->is_tooltip) close_window(c, (int)i);
        if(!w->window) take_spare(c, (int)i, l->is_tooltip);
        if(!w->window)
        {
            w->tooltip = l->is_tooltip;
            SDL_PropertiesID props = SDL_CreateProperties();
            SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_PARENT_POINTER, p->window);
            SDL_SetNumberProperty (props, SDL_PROP_WINDOW_CREATE_X_NUMBER, x);
            SDL_SetNumberProperty (props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, y);
            SDL_SetNumberProperty (props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, ww);
            SDL_SetNumberProperty (props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, wh);
            SDL_SetNumberProperty (props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, SDL_WINDOW_TRANSPARENT | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                                   (l->is_tooltip ? SDL_WINDOW_TOOLTIP : SDL_WINDOW_POPUP_MENU));
#ifdef SDL_PROP_WINDOW_CREATE_CONSTRAIN_POPUP_BOOLEAN
            SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_CONSTRAIN_POPUP_BOOLEAN, false);
#endif
            w->window = create_window(c, props);
            if(!w->window)
            {
                c->no_popup_windows = true;
                draw_into_domain(c, (int)i);
                continue;
            }
        }
        else if(px != w->x || py != w->y || ww != w->w || wh != w->h)
        {
            SDL_SetWindowPosition(w->window, x, y);
            SDL_SetWindowSize(w->window, ww, wh);
        }
        if(w->hidden) { SDL_ShowWindow(w->window); w->hidden = false; }
        w->x = px; w->y = py;
        w->w = ww; w->h = wh;
    }
    for(int i = 1; i < RCUT_SDL_MAX_WINDOWS; i++)
    {
        r_sdl_window* w = &c->windows[i];
        if(state[i] == 2 || !w->window) continue;
        if(w->domain == i) close_window(c, i);
        else if(!w->hidden) { SDL_HideWindow(w->window); w->hidden = true; w->last_window_surface = NULL; }
    }
}

static uint32_t map_modifiers(SDL_Keymod m)
{
    uint32_t r = 0;
    if(m & SDL_KMOD_CTRL)  r |= R_MOD_CTRL;
    if(m & SDL_KMOD_ALT)   r |= R_MOD_ALT;
    if(m & SDL_KMOD_SHIFT) r |= R_MOD_SHIFT;
    if(m & SDL_KMOD_GUI)   r |= R_MOD_WIN;
    return r;
}

static int32_t map_key(SDL_Keycode k)
{
    if(k >= 'a' && k <= 'z') return (int32_t)(k - 'a' + 'A');
    if(k >= '0' && k <= '9') return (int32_t)k;
    switch(k)
    {
        case SDLK_BACKSPACE: return R_KEY_BACKSPACE;
        case SDLK_TAB:       return R_KEY_TAB;
        case SDLK_RETURN:    return R_KEY_ENTER;
        case SDLK_KP_ENTER:  return R_KEY_ENTER;
        case SDLK_LALT:      return R_KEY_ALT; // Right Alt is AltGr on many layouts, it doesn't enter accelerator mode.
        case SDLK_ESCAPE:    return R_KEY_ESCAPE;
        case SDLK_SPACE:     return R_KEY_SPACE;
        case SDLK_PAGEUP:    return R_KEY_PAGEUP;
        case SDLK_PAGEDOWN:  return R_KEY_PAGEDOWN;
        case SDLK_END:       return R_KEY_END;
        case SDLK_HOME:      return R_KEY_HOME;
        case SDLK_LEFT:      return R_KEY_LEFT;
        case SDLK_UP:        return R_KEY_UP;
        case SDLK_RIGHT:     return R_KEY_RIGHT;
        case SDLK_DOWN:      return R_KEY_DOWN;
        case SDLK_DELETE:    return R_KEY_DELETE;
    }
    return (int32_t)(k | 0x01000000);
}

static bool to_target(r_sdl_frontend* c, SDL_WindowID id, float* x, float* y, uint64_t* target)
{
    int i = window_index(c, id);
    if(i < 0) return false;
    float density = SDL_GetWindowPixelDensity(c->windows[i].window);
    *x = (*x * density + c->windows[i].x) / c->draw.scale;
    *y = (*y * density + c->windows[i].y) / c->draw.scale;
    *target = (uint64_t)c->windows[i].domain;
    return true;
}

static uint64_t key_target(r_sdl_frontend* c, SDL_WindowID id)
{
    int i = window_index(c, id);
    return i < 0 ? 0 : (uint64_t)c->windows[i].domain;
}

bool r_sdl_process_event(r_sdl_frontend* c, SDL_Event* e)
{
    r_context* ctx = &c->context;
    float x, y;
    uint64_t t;
    int i;
    switch(e->type)
    {
        case SDL_EVENT_QUIT:
            c->quit_requested = true;
            break;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            i = window_index(c, e->window.windowID);
            if(i == 0) c->quit_requested = true;
            if(i > 0 && c->windows[i].domain == i) c->windows[i].close_requested = true; // A dialog, it's up to the application.
            else if(i > 0) // A popup: closed.
            for(r_ui_list* l = ctx->chain_start; l; l = l->chain_next)
                if(l->target == (uint64_t)i && l->ephemeral_parent && l->ephemeral_parent->target != l->target)
                    r_ephemeral_mark_to_delete(l->ephemeral_parent, l->root_area.id);
            break;
        case SDL_EVENT_WINDOW_DESTROYED:
            if(window_index(c, e->window.windowID) == 0) c->quit_requested = true;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            x = e->motion.x; y = e->motion.y;
            return to_target(c, e->motion.windowID, &x, &y, &t) && r_post_mouse_move(ctx, t, x, y) != 0;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            x = e->button.x; y = e->button.y;
            return to_target(c, e->button.windowID, &x, &y, &t) && r_post_mouse_down(ctx, t, e->button.button, x, y) != 0;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            x = e->button.x; y = e->button.y;
            return to_target(c, e->button.windowID, &x, &y, &t) && r_post_mouse_up(ctx, t, e->button.button, x, y) != 0;
        case SDL_EVENT_MOUSE_WHEEL:
        {
            float dir = e->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
            x = e->wheel.mouse_x; y = e->wheel.mouse_y;
            return to_target(c, e->wheel.windowID, &x, &y, &t) && r_post_mouse_wheel(ctx, t, x, y, e->wheel.x * dir, e->wheel.y * dir) != 0;
        }
        case SDL_EVENT_KEY_DOWN:
            return r_post_key_input(ctx, key_target(c, e->key.windowID), map_key(e->key.key), (int32_t)e->key.scancode, map_modifiers(e->key.mod)) != 0;
        case SDL_EVENT_TEXT_INPUT:
            return r_post_text(ctx, key_target(c, e->text.windowID), e->text.text) != 0;
        case SDL_EVENT_TEXT_EDITING:
            return r_post_text_edit(ctx, key_target(c, e->edit.windowID), e->edit.text, e->edit.start, e->edit.length) != 0;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            if(window_index(c, e->window.windowID) == 0) update_root_sizes(c);
            return false;
        case SDL_EVENT_WINDOW_EXPOSED:
            i = window_index(c, e->window.windowID);
            if(i >= 0) c->windows[i].last_window_surface = NULL;
            return true;
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
            if(c->auto_scale && window_index(c, e->window.windowID) == 0) r_sdl_set_scale(c, 0);
            return false;
    }
    return false;
}

// Rendering

static void draw_into(r_sdl_frontend* c, int i, SDL_Surface* s, rect area)
{
    r_sdl_window* w = &c->windows[i];
    uint32_t* pixels = (uint32_t*)s->pixels + (area.y - w->y) * (s->pitch / 4) + (area.x - w->x);
    r_draw_software_set_target(&c->software, pixels, area.w, area.h, s->pitch / 4, area.x, area.y);
    c->software.clear_color = w->domain == i ? c->draw.clear_color : 0;
    r_draw_render_area(&c->draw, (uint64_t)i, area);
}

static bool keeps_contents(r_sdl_window* w, void* surface, int pw, int ph)
{
    bool kept = w->last_window_surface == surface && w->drawn_x == w->x && w->drawn_y == w->y && w->drawn_w == pw && w->drawn_h == ph;
    w->last_window_surface = surface; w->drawn_x = w->x; w->drawn_y = w->y; w->drawn_w = pw; w->drawn_h = ph;
    return kept;
}

static void render_window(r_sdl_frontend* c, int i)
{
    r_sdl_window* w = &c->windows[i];
    rect damage = r_draw_take_damage(&c->draw, (uint64_t)i);
    SDL_Surface* surface = SDL_GetWindowSurface(w->window), *s = surface;
    if(!surface) return;
    if(surface->format != SDL_PIXELFORMAT_ARGB8888 && surface->format != SDL_PIXELFORMAT_XRGB8888)
    {
        if(!w->internal_surface || w->internal_surface->w != surface->w || w->internal_surface->h != surface->h)
        {
            SDL_DestroySurface(w->internal_surface);
            w->internal_surface = SDL_CreateSurface(surface->w, surface->h, SDL_PIXELFORMAT_ARGB8888);
            if(!w->internal_surface) return;
            SDL_SetSurfaceBlendMode(w->internal_surface, SDL_BLENDMODE_NONE);
        }
        s = w->internal_surface;
    }
    if(i == 0 && c->vsync && surface != w->last_window_surface) SDL_SetWindowSurfaceVSync(w->window, 1);
    rect whole = { w->x, w->y, s->w, s->h };
    damage = keeps_contents(w, surface, s->w, s->h) ? rect_intersect(damage, whole) : whole;
    if(damage.w <= 0 || damage.h <= 0) return;
    draw_into(c, i, s, damage);
    SDL_Rect r = { damage.x - w->x, damage.y - w->y, damage.w, damage.h };
    if(s != surface) SDL_BlitSurface(s, &r, surface, &r);
    SDL_UpdateWindowSurfaceRects(w->window, &r, 1);
}

void r_sdl_render_ui(r_sdl_frontend* c)
{
    update_windows(c);
    for(int i = 0; i < RCUT_SDL_MAX_WINDOWS; i++)
        if(c->windows[i].window && !c->windows[i].hidden) render_window(c, i);
    r_damage_clear(&c->context);
}

bool r_sdl_screenshot(r_sdl_frontend* c, const char* path)
{
    r_sdl_render_ui(c);
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(c->windows[0].window, &w, &h);
    SDL_Surface* s = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_ARGB8888);
    if(!s) return false;
    r_draw_software_set_target(&c->software, s->pixels, s->w, s->h, s->pitch / 4, 0, 0);
    c->software.clear_color = c->draw.clear_color;
    r_draw_render(&c->draw, 0);
    if(c->draw.cache_full || c->draw.glyphs_full) r_draw_render(&c->draw, 0);
#if SDL_VERSION_ATLEAST(3, 4, 0)
    bool ok = SDL_SavePNG(s, path);
#else
    bool ok = SDL_SaveBMP(s, path);
#endif
    SDL_DestroySurface(s);
    return ok;
}
