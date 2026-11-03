#include <SDL3/SDL.h>
#include "rcut_sdl.h"

#define COUNTOF(a) ((int)(sizeof(a) / sizeof((a)[0])))

r_ui_list* window_ui;

typedef struct preview
{
    const char* name;
    int style;
    int w, h;
    int icon;
    const char* text;
} preview;

#define SECTION(NAME)           { NAME, -1 }
#define ITEM(STYLE, W, H, TEXT) { #STYLE, STYLE, W, H, R_ICON_NONE, TEXT }
#define ICON(STYLE, W, H, ICON) { #STYLE, STYLE, W, H, ICON, NULL }

static const preview to_display[] = {
    SECTION("Text"),
    ITEM(R_STYLE_HEADER, 0, 22, "Header"),
    ITEM(R_STYLE_LABEL, 0, 20, "Label"),
    ITEM(R_STYLE_LABEL | R_VARIANT_DULL, 0, 20, "Dull label"),
    ITEM(R_STYLE_LABEL | R_VARIANT_BOLD, 0, 20, "Bold label"),

    SECTION("Buttons"),
    ITEM(R_STYLE_BUTTON | R_VARIANT_BOLD, 160, 0, "Primary"),
    ITEM(R_STYLE_BUTTON | R_VARIANT_BOLD | R_STATE_HOVER, 160, 0, "Primary"),
    ITEM(R_STYLE_BUTTON | R_VARIANT_BOLD | R_STATE_PRESSED, 160, 0, "Primary"),
    ITEM(R_STYLE_BUTTON, 160, 0, "Button"),
    ITEM(R_STYLE_BUTTON | R_STATE_HOVER, 160, 0, "Button"),
    ITEM(R_STYLE_BUTTON | R_STATE_PRESSED, 160, 0, "Button"),
    ITEM(R_STYLE_BUTTON | R_STATE_DISABLED, 160, 0, "Button"),
    ITEM(R_STYLE_BUTTON | R_VARIANT_DULL, 160, 0, "Flat"),
    ITEM(R_STYLE_BUTTON | R_VARIANT_DULL | R_STATE_HOVER, 160, 0, "Flat"),
    ITEM(R_STYLE_BUTTON | R_COMPONENT_ENDCAP_START, 80, 0, "Start"),
    ITEM(R_STYLE_BUTTON | R_COMPONENT_JOINED, 80, 0, "Joined"),
    ITEM(R_STYLE_BUTTON | R_COMPONENT_JOINED | R_STATE_TOGGLED, 80, 0, "Selected"),
    ITEM(R_STYLE_BUTTON | R_COMPONENT_JOINED | R_STATE_FOCUS, 80, 0, "Focused"),
    ITEM(R_STYLE_BUTTON | R_COMPONENT_JOINED | R_STATE_TOGGLED | R_STATE_FOCUS, 80, 0, "Focused"),
    ITEM(R_STYLE_BUTTON | R_COMPONENT_ENDCAP_END, 80, 0, "End"),
    ITEM(R_STYLE_DROPDOWN, 160, 0, "Dropdown"),
    ITEM(R_STYLE_DROPDOWN | R_STATE_HOVER, 160, 0, "Dropdown"),
    ITEM(R_STYLE_DROPDOWN | R_STATE_TOGGLED, 160, 0, "Dropdown"),

    SECTION("Text fields"),
    ITEM(R_STYLE_INPUT, 200, 0, "Text"),
    ITEM(R_STYLE_INPUT | R_STATE_HOVER, 200, 0, "Text"),
    ITEM(R_STYLE_INPUT | R_STATE_FOCUS, 200, 0, "Text"),
    ITEM(R_STYLE_INPUT | R_VARIANT_DULL, 200, 0, "Placeholder"),
    ITEM(R_STYLE_INPUT | R_COMPONENT_SELECTION, 60, 18, NULL),
    ITEM(R_STYLE_INPUT | R_COMPONENT_CARET, 2, 18, NULL),
    ITEM(R_STYLE_INPUT_MULTILINE_EDIT, 200, 64, "Multiple\nlines"),
    ITEM(R_STYLE_SPINBOX, 120, 0, "42"),
    ITEM(R_STYLE_SLIDEBOX, 200, 0, "Value: 50"),
    ITEM(R_STYLE_SLIDEBOX | R_COMPONENT_FILL, 100, 30, NULL),

    SECTION("Indicators"),
    ITEM(R_STYLE_CHECKBOX, 18, 18, NULL),
    ITEM(R_STYLE_CHECKBOX | R_STATE_HOVER, 18, 18, NULL),
    ICON(R_STYLE_CHECKBOX | R_STATE_TOGGLED, 18, 18, R_ICON_CHECKED),
    ITEM(R_STYLE_RADIO, 18, 18, NULL),
    ICON(R_STYLE_RADIO | R_STATE_TOGGLED, 18, 18, R_ICON_MARKED),
    ITEM(R_STYLE_TOGGLE | R_COMPONENT_TRACK, 32, 18, NULL),
    ITEM(R_STYLE_TOGGLE | R_COMPONENT_TRACK | R_STATE_TOGGLED, 32, 18, NULL),
    ITEM(R_STYLE_TOGGLE | R_COMPONENT_THUMB, 14, 14, NULL),

    SECTION("Sliders, progress, scrollbars"),
    ITEM(R_STYLE_SLIDER | R_COMPONENT_TRACK, 200, 5, NULL),
    ITEM(R_STYLE_SLIDER | R_COMPONENT_FILL, 120, 5, NULL),
    ITEM(R_STYLE_SLIDER | R_COMPONENT_THUMB, 16, 16, NULL),
    ITEM(R_STYLE_SLIDER | R_COMPONENT_THUMB | R_STATE_HOVER, 16, 16, NULL),
    ITEM(R_STYLE_PROGRESS | R_COMPONENT_TRACK, 200, 6, NULL),
    ITEM(R_STYLE_PROGRESS | R_COMPONENT_FILL, 120, 6, NULL),
    ITEM(R_STYLE_SCROLLBAR | R_COMPONENT_TRACK, 10, 80, NULL),
    ITEM(R_STYLE_SCROLLBAR | R_COMPONENT_THUMB, 10, 40, NULL),
    ITEM(R_STYLE_SCROLLBAR | R_COMPONENT_THUMB | R_STATE_HOVER, 10, 40, NULL),
    ITEM(R_STYLE_SCROLLBAR | R_COMPONENT_THUMB | R_STATE_PRESSED, 10, 40, NULL),

    SECTION("Containers"),
    ITEM(R_STYLE_PANEL, 200, 64, NULL),
    ITEM(R_STYLE_PANEL | R_VARIANT_RAISED, 200, 64, NULL),
    ITEM(R_STYLE_DIALOG, 200, 80, NULL),
    ITEM(R_STYLE_BACKDROP, 200, 40, NULL),
    ITEM(R_STYLE_TOOLTIP, 100, 24, "Tooltip"),
    ITEM(R_STYLE_SEPARATOR, 200, 1, NULL),
    ITEM(R_STYLE_SEPARATOR | R_STATE_HOVER, 200, 1, NULL),

    SECTION("Menus"),
    ITEM(R_STYLE_MENUBAR_BG, 200, 30, NULL),
    ITEM(R_STYLE_MENUBAR_ITEM, 60, 24, "File"),
    ITEM(R_STYLE_MENUBAR_ITEM | R_STATE_HOVER, 60, 24, "File"),
    ITEM(R_STYLE_MENU_BG, 160, 64, NULL),
    ITEM(R_STYLE_MENU_ITEM, 160, 26, "Open"),
    ITEM(R_STYLE_MENU_ITEM | R_STATE_HOVER, 160, 26, "Open"),
    ITEM(R_STYLE_MENU_ITEM | R_STATE_SELECTED, 160, 26, "Open"),
    ITEM(R_STYLE_MENU_ITEM | R_VARIANT_DULL, 160, 26, "Ctrl+O"),

    SECTION("Tabs and lists"),
    ITEM(R_STYLE_TAB_BAR, 200, 32, NULL),
    ITEM(R_STYLE_TAB, 80, 32, "Tab"),
    ITEM(R_STYLE_TAB | R_STATE_HOVER, 80, 32, "Tab"),
    ITEM(R_STYLE_TAB | R_STATE_SELECTED, 80, 32, "Tab"),
    ITEM(R_STYLE_TAB_PAGE, 200, 64, NULL),
    ITEM(R_STYLE_LIST_ITEM, 200, 28, "Item"),
    ITEM(R_STYLE_LIST_ITEM | R_STATE_HOVER, 200, 28, "Item"),
    ITEM(R_STYLE_LIST_ITEM | R_STATE_SELECTED, 200, 28, "Item"),
    ITEM(R_STYLE_COLLAPSIBLE, 200, 28, "Section"),
    ITEM(R_STYLE_COLLAPSIBLE | R_STATE_HOVER, 200, 28, "Section"),
};

static int built_w, built_h;

void rebuild_ui(r_sdl_frontend* f)
{
    r_ui_list* l = window_ui;
    built_w = l->root_area.w; built_h = l->root_area.h;
    r_list_clear(l);
    r_ui_list* s = r_scroll_begin_ex(l, l->root_area, 0);
    rect document = r_shrink(s->document_area, 16);
    document.h = 100000;
    r_layout_iter rows = r_stack(s, document, R_TOP);
    rows.spacing = 0;
    for(int i = 0; i < COUNTOF(to_display); i++)
    {
        const preview* p = &to_display[i];
        if(p->style == -1)
        {
            r_next(&rows, 12);
            r_header(s, r_next(&rows, 32), (char*)p->name);
            continue;
        }

        int h = p->h ? p->h : r_layout_rows(s, document, p->style).size;
        r_layout_iter row = r_row(&rows, (h > 20 ? h : 20) + 12, R_LEFT);
        rect column = r_next(&row, 240);
        rect box = { .w = p->w ? p->w : column.w, .h = h };
        r_align(R_LEFT_CENTER, column, &box);
        r_drawable* d = p->text ? r_labeled(s, box, p->style, p->text) : r_style(s, box, p->style);
        d->icon = p->icon;
        r_label_ex(s, r_remaining(&row), R_STYLE_LABEL | R_VARIANT_DULL, R_ICON_NONE, (char*)p->name);
    }
    s->document_area.h = rows.start.h - rows.area.h + 32;
    r_scroll_end(l, s);
}

int main(int argc, char** argv)
{
    r_sdl_frontend* f = r_sdl_init("rcut styles", 800, 720, R_SDL_RESIZABLE);
    if(!f) return 1;

    r_theme_default(&f->draw, &(r_draw_theme_options){.background=r_rgb_u8(242, 242, 242)});

    window_ui = f->root;
    rebuild_ui(f);

    while(!r_sdl_quit_requested(f, true))
    {
        SDL_Event e;
        if(SDL_WaitEventTimeout(&e, 50))
        {
            do r_sdl_process_event(f, &e); while(SDL_PollEvent(&e));
        }
        if(window_ui->root_area.w != built_w || window_ui->root_area.h != built_h) rebuild_ui(f);
        r_sdl_update(f);
        r_sdl_render_ui(f);
    }
    r_sdl_free(f);
    return 0;
}
