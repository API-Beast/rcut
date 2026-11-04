# 1.0 - Quick Start

```c
#include <SDL3/SDL.h>
#include "rcut_sdl.h"

static bool quit;

static void on_ok(void)
{
	quit = true;
}

int main(void)
{
	r_sdl_frontend* f = r_sdl_init("rcut tutorial", 320, 150, 0);
	if(!f) return 1;
	r_ui_list* ui = f->root;

	rect ok = { 110, 75, 100, 50, .id = 2 };
	r_label(ui, (rect){ 63, 0, 194, 75, .id = 1 }, "Hello World from rcut! 😎👍");
	r_button(ui, ok, "OK");
	r_on_click(ui, ok, on_ok);

	while(!quit && !r_sdl_quit_requested(f, true))
	{
		SDL_Event e;
		if(SDL_WaitEventTimeout(&e, 50))
		{
			do r_sdl_process_event(f, &e); while(SDL_PollEvent(&e));
		}
		r_sdl_update(f);
		r_sdl_render_ui(f);
	}
	r_sdl_free(f);
	return 0;
}
```
<-- screenshot -->

Explanation ui elements just being rectangles stored in a list.

Explanation rcut_sdl/rcut_draw/rcut_core split.

Explanation ids.

Note not being intermediate mode.

## 1.1 - Data binding

<-- Replace label with slider and reactive label, show one example for every type of binding. -->

## 1.2 - Layout Iterators

In the main function:
<!-- TODO: Rework example to build multiple rows from an array. -->
```c
    r_layout_iter vertical = r_stack(ui, ui->root_area, R_TOP);
    
    rect row = r_next(&vertical, 32); // One 32px row.
    r_layout_iter horizontal = r_stack(ui, row, R_LEFT);
    r_slider_box(ui, r_next_fraction(&row, 0.5), "Value", 0.0f, 1.0f, 0.01f, R_PTR(&value));
    r_sparkline(ui, r_remaining(&row), 0.0f, 1.0f, R_PTR(&value));
    r_next(&vertical, 16); // Some spacing
    rect bottom = r_remaining(&vertical);
    rect button = r_aligned(R_CENTER, row, (rect){.w=100, .h=32});
    r_button(ui, button, "OK");
    r_on_click(ui, button, &on_button_press);
```
<-- screenshot -->

Explanation:
- r_slider_box
- r_stack, r_next, r_next_fraction
- r_remaining
- r_aligned

## 1.3 - Resizable UIs

<-- Explain to move UI building into a seperate function and rebuild when the UI list is marked. -->

## 1.4 - Rectangle Cutting

<-- code example: panel with menu bar, status bar, main area, side bar
    rect main = ui->root_area;
    rect menu = r_cut_px(R_TOP, &main, 32, 1);
    rect status
    rect side = r_cut_
    // Main was modified by the r_cut* calls and only contains the remaining area.
    
    r_style(ui, main, )
    r_label(ui, main, "main")
    r_style(menu)

-->
<-- screenshot -->

## 1.5 - Scrollables & Tabs

## 1.6 - Tooltips

## 1.7 - r_style and custom themes

## 1.8 - Menus

## 1.9 - List of UI widgets
Simple list with links to API documentation of each.

# 2.0 - Implementing your own widgets

Explain core concepts.

## 2.1 - UI Lists
## 2.2 - Drawables

### 2.2.1 - Style fallback rules

## 2.3 - Interactables and Callbacks

### 2.3.1 - Activation
### 2.3.2 - Mouse events
### 2.3.3 - Drag events
### 2.3.4 - Misc events
### 2.3.5 - Context menu: meta events

## 2.5 - Dynamic Drawables: Toggle Button

## 2.6 - Dynamic Popups: Ephemeral Lists

# 3.0 - Behind the scenes

Seperation between rcut_core, rcut_draw and rcut_sdl.

## 3.1 - Integrating rcut_draw into your own engine.

## 3.2 - Using rcut_core without rcut_draw.