#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define RCUT_MOUSE_DRAG_DISTANCE_DEFAULT 4
// In seconds
#define RCUT_MOUSE_PRESSED_INITIAL_REPEAT 0.5
#define RCUT_MOUSE_PRESSED_REPEAT 0.2
#define RCUT_MOUSE_HOVER_INITIAL_DELAY 0.4
#define RCUT_MOUSE_HOVER_REPEAT  0.2
#define RCUT_MOUSE_MULTI_CLICK_TIME 0.4

// This is the maximum number of elements a r_ui_list can grow to.
// This much virtual memory will be reserved. It will only be commited if needed.
// Reserved virtual memory is a limited resource on 32 bit systems, but basically free on 64 systems, hence the different limits.
#if UINTPTR_MAX == 0xffffffff           // 32 bit virtual memory
#define RCUT_CONTEXT_MAX_LISTS 128
#define RCUT_CONTEXT_MAX_EPHEMERAL_LISTS 128
#define RCUT_LIST_MAX_ELEMENTS 4096  // ~4 millionths of address space per r_ui_list
#define RCUT_EPHEMERAL_MAX_ELEMENTS 256 // Tooltips, all other lists use RCUT_LIST_MAX_ELEMENTS
#elif UINTPTR_MAX == 0xffffffffffffffff // 64 bit virtual memory
#define RCUT_CONTEXT_MAX_LISTS 256
#define RCUT_CONTEXT_MAX_EPHEMERAL_LISTS 256
#define RCUT_LIST_MAX_ELEMENTS 65535 // ~1 billionths of address space per r_ui_list
#define RCUT_EPHEMERAL_MAX_ELEMENTS 512 // Tooltips, all other lists use RCUT_LIST_MAX_ELEMENTS
#endif

#define RCUT_HIT_STACK_MAX 32

typedef struct rect {
	int x, y, w, h; uint64_t id;
} rect;

// Keys that are special cased by rcut. (Windows virtual key codes, frontends need to map their keys to these.)
// Letters are expected as uppercase ASCII ('A'-'Z'), digits as ASCII.
#define R_KEY_BACKSPACE 8
#define R_KEY_TAB       9
#define R_KEY_ENTER     13
#define R_KEY_ALT       18
#define R_KEY_ESCAPE    27
#define R_KEY_SPACE     32
#define R_KEY_PAGEUP    33
#define R_KEY_PAGEDOWN  34
#define R_KEY_END       35
#define R_KEY_HOME      36
#define R_KEY_LEFT      37
#define R_KEY_UP        38
#define R_KEY_RIGHT     39
#define R_KEY_DOWN      40
#define R_KEY_DELETE    46

// Mouse buttons (same numbering as SDL)
#define R_MOUSE_LEFT   1
#define R_MOUSE_MIDDLE 2
#define R_MOUSE_RIGHT  3

#define R_BIT(x) (1u << (x))
enum r_style
{
    R_STYLE_FALLBACK = 0, // If the style doesn't know how to draw something, it should fallback to drawing this instead.
    R_STYLE_WINDOW,
    R_STYLE_TOOLTIP,
    R_STYLE_BUTTON,
    R_STYLE_HEADER,
    R_STYLE_LABEL,
    R_STYLE_ICON,
    R_STYLE_TEXT,
    R_STYLE_INPUT, // Text areas with various misc functionalities
	R_STYLE_MENU_BG,
    R_STYLE_MENU_ITEM,
    R_STYLE_CONTEXT_MENU_BG,
    R_STYLE_CONTEXT_MENU_ITEM,
    R_STYLE_PANEL,
    R_STYLE_CHECKBOX,
    R_STYLE_RADIO,
    R_STYLE_TOGGLE,
    R_STYLE_SLIDEBOX, // Mix between slider and input box, with both numerical and drag input.
    R_STYLE_SLIDER,
    R_STYLE_SCROLLBAR,
    R_STYLE_MENUBAR_BG,
    R_STYLE_MENUBAR_ITEM,
    R_STYLE_INPUT_LINE_EDIT,
    R_STYLE_INPUT_MULTILINE_EDIT,
    R_STYLE_PROGRESS,
    R_STYLE_TAB,
    R_STYLE_TAB_BAR,
    R_STYLE_SEPARATOR,
    R_STYLE_COLLAPSIBLE,  // Header of a collapsible section.
    R_STYLE_LIST_ITEM,    // Selectable item in a list.
    R_STYLE_DROPDOWN,     // Button that opens a selection popup.
    R_STYLE_SPINBOX,
    R_STYLE_BACKDROP,     // Fills a window behind something modal. Unused since dialogs are root lists of their own.
    R_STYLE_DIALOG,
    R_STYLE_TAB_PAGE,     // Background of the content below tabs.
    R_STYLE_SEGMENTED,    // Joined choices, one is selected.
    R_STYLE_BASE_MASK = 0xFF,
    
    R_STATE_SELECTED  = R_BIT(0) << 8, // Selection in a menu.
    R_STATE_PRESSED   = R_BIT(1) << 8,
    R_STATE_HOVER     = R_BIT(2) << 8,
    R_STATE_DISABLED  = R_BIT(3) << 8,
    R_STATE_TOGGLED   = R_BIT(4) << 8,
	R_STATE_FOCUS     = R_BIT(5) << 8,
	// Up to R_BIT(15) << 8
    R_STATE_MASK = 0xFFFF << 8,

    R_VARIANT_FLAT            = 0 << 24, // Usually means without border and the element is just implied through icon + label
    R_VARIANT_RAISED          = 1 << 24, // Raised panel, by default panels are inset.
    R_VARIANT_HORZ            = 2 << 24, // Horizontal scrollbar/slider
	R_VARIANT_VERT            = 3 << 24, // Vertical scrollbar/slider
    R_VARIANT_BOLD            = 4 << 24, // Primary button - More colorful
    R_VARIANT_DULL            = 5 << 24, // Tertiary button - Less color, flatter
	R_VARIANT_LISTITEM        = 6 << 24, // UI element is part of a list, that usually implies having the same background as the list and a flat appearance.
    R_VARIANT_MASK            = 0xF << 24,

	R_COMPONENT_JOINED        = 1 << 28, // Part of an complex input, wedged inbetween endcaps.
    R_COMPONENT_ENDCAP_START  = 2 << 28, // For example to create a text input with a popup selection then we have:
    R_COMPONENT_ENDCAP_END    = 3 << 28, // [R_STYLE_INPUT | R_COMPONENT_ENDCAP_START][R_STYLE_BUTTON | R_COMPONENT_ENDCAP_END]

	R_COMPONENT_TRACK         = 4 << 28, // Primary background for progress bars/sliders/scroll bar, behind joined components (with the style of the joined parts).
    R_COMPONENT_FILL          = 5 << 28, // Alternative background for progress bars that indicate the progress.
    R_COMPONENT_THUMB         = 6 << 28, // Draggable portion of slider/scroll bar.
    R_COMPONENT_CARET         = 7 << 28, // Text cursor, blinks while focused.
    R_COMPONENT_SELECTION     = 8 << 28, // Text selection highlight.

    R_COMPONENT_MASK          = 0xF << 28,
};

typedef enum r_icon_variant{
	R_ICON_NONE,
	R_ICON_DROPDOWN_INDICATOR,
	R_ICON_UP,
	R_ICON_DOWN,
	R_ICON_LEFT,
	R_ICON_RIGHT,
	R_ICON_BACK, // <-
	R_ICON_MORE, // ...
	R_ICON_EXPANDED, //  [-]
	R_ICON_COLLAPSED, // [+]
	R_ICON_SHOW_DETAILS, // >
	R_ICON_HIDE_DETAILS, // ▼
	R_ICON_ADD, // +
	R_ICON_SUBTRACT, // -
	R_ICON_EDIT,
	R_ICON_SEARCH,
	R_ICON_RELOAD,
	R_ICON_ZOOM_IN,
	R_ICON_ZOOM_OUT,
	R_ICON_ROTATE_LEFT,
	R_ICON_ROTATE_RIGHT,
	R_ICON_LOCK,
	R_ICON_UNLOCK,
	R_ICON_CHECKED,
	R_ICON_UNCHECKED,
	R_ICON_MARKED,
	R_ICON_INFO, // (?)
	R_ICON_WARNING, // (!)
	R_ICON_ERROR, // (x)
	R_ICON_MINIMIZE, // _
	R_ICON_MAXIMIZE,
	R_ICON_UNMAXIMIZE,
	R_ICON_CLOSE, // x
	R_ICON_MENU, // Hamburger
	R_ICON_SETTINGS,
	R_ICON_HOME,
	R_ICON_FOLDER,
	R_ICON_FILE,
	R_ICON_SAVE,
	R_ICON_TRASH,
	R_ICON_COPY,
	R_ICON_CUT,
	R_ICON_PASTE,
	R_ICON_UNDO,
	R_ICON_REDO,
	R_ICON_STAR,
	R_ICON_HEART,
	R_ICON_BELL,
	R_ICON_PLAY,
	R_ICON_PAUSE,
	R_ICON_EYE,
	R_ICON_PERSON,
	R_ICON_USER
} r_icon_variant;

typedef enum r_event_type
{
	R_ACTIVATION,  // Sent on mouse_click or via a Enter keypress if focused.
	// Also sent when Alt was pressed to enter menu accelerator mode and then the interactables accelerator key was pressed.
	R_CONTEXT,     // Triggered when it's label is selected in the context menu, or when it's hotkey is pressed while this element is focused.

	R_TEXT_INPUT,  // Only sent if in focus and if has_text is true.
	R_TEXT_EDIT,   // IME input: show a transient text fragment with selection.
	R_KEY_INPUT,   // Only sent if in focus. Tabs and arrow key events will only be sent if not in navigation mode and traps_tab_focus/traps_arrow_focus is true.
	R_MOUSE_IN,
	R_MOUSE_HOVER, // For popups primarily. Mouse hover is sent after MOUSE_HOVER_INITIAL_DELAY (Default: 0.4) seconds after the R_MOUSE_IN event, then repeated every MOUSE_HOVER_REPEAT_DELAY (Default: 0.2) seconds.
	R_MOUSE_OUT,
	R_MOUSE_CLICK,      // Mouse click is sent on mouse release and is never repeated.
	R_MOUSE_PRESSED,    // Mouse pressed is sent once during the initial mouse_down, then after MOUSE_PRESSED_INITIAL_REPEAT seconds then repeated every MOUSE_PRESSED_REPEAT seconds.
	R_MOUSE_DRAG_START, // Drags are stateful: Whichever interactable got the drag_start_ event will also get the drag_ and drag_release_ events, regardless of mouse position. 
	R_MOUSE_DRAG_MOVE,  // Drag start/drag release events will always be accompanied by drag move events.
	R_MOUSE_DRAG_RELEASE,
	R_MOUSE_SCROLL,
	R_FOCUS_ENTER,
	R_FOCUS_EXIT,
} r_event_type;

typedef struct r_drawable r_drawable;
typedef struct r_callback r_callback;
typedef struct r_ui_list r_ui_list;
typedef struct r_context r_context;

typedef enum r_cursor
{
	R_CURSOR_DEFAULT,
	R_CURSOR_TEXT,
	R_CURSOR_HAND,
	R_CURSOR_RESIZE_H, // Left-right
	R_CURSOR_RESIZE_V, // Up-down
} r_cursor;

typedef struct r_interact
{
	r_ui_list* owner;

	rect r;
	r_cursor cursor;

	bool traps_tab_focus   : 1; // If true, you can't use tab to change focus while this element has keyboard focus.
	bool traps_alt_key     : 1; // If true, you can't activate acclerator hotkeys while this element has keyboard focus.
	bool traps_arrow_focus : 1; // If true, you can't use arrow keys to change the focus after pressing tab while this element has keyboard focus.
	
	int accelerator_key; // Alt + this key will send a ACTIVATION event to this interactable if not blocked.

	// Mouse-capture group. While captured, only interactables whose press_group
	// matches the captured group (or whose id == pressed_id) receive mouse events.
	// 0 means "only myself". Menu items set this to the menu button's group id.
	uint64_t press_group;

	r_callback* callbacks; // Linked list

	// Tab focusing. These are overrides, if NULL a default value will be computed.
	struct r_interact* next_focus;     // Switch to this element on pressing Tab.
	struct r_interact* previous_focus; // Switch to this element on pressing Shift-Tab.
	// Arrow key focus.
	struct r_interact* directional_focus[4]; // Switch to this element when pressing Up/Left/Down/Right

	struct r_dynamic_rect_data* dynamic;
} r_interact;

typedef struct r_drawable
{
	rect r;
	uint64_t parent_id; // Inherits hovered/pressed/focus state and damage state from parent_id. Will be 0 most of the time.

	int style;       // r_style
	int outer_style; // A style hint to allow for minor style adjustments for nested drawables. Will be 0 most of the time.
	// Label = Text + Icon
	char* text;
	int   icon;
	int16_t label_offset_x, label_offset_y;

	void* layout_cache; // The actual positions of each glyph, mainly used for shaped text.
	bool  wrap_label : 1;

	struct r_dynamic_rect_data* dynamic;
} r_drawable;

typedef enum r_modifier_flags
{
	R_MOD_CTRL  = (1 << 0),
	R_MOD_ALT   = (1 << 1),
	R_MOD_SHIFT = (1 << 2),
	R_MOD_WIN   = (1 << 3)
} r_modifier_flags;

typedef struct r_event
{
	r_ui_list* ui_list;
	r_context* context;
	int mouse_button;
	int mouse_state;
	int click_count;
	// The three coordinate spaces:
	// Target space: The mouse position relative to the render-target/window.
	float target_mouse_x, target_mouse_y;
	// List space: After applying the transformation matrix of the UI list.
	float list_mouse_x, list_mouse_y;
	// Rectangle space: List space coordinates relative to rectangle X/Y.
	float rect_mouse_x, rect_mouse_y;
	// Drag coordinates are in List space.
	float drag_start_x, drag_start_y;
	float drag_delta_x, drag_delta_y;
	float scroll_x, scroll_y;
	// Keyboard
	int keycode;
	int scancode;
	int modifier_state;
	// Text input
	char* text; // UTF-8 fragment. R_TEXT_INPUT needs to append it, R_TEXT_EDIT needs to display it without committing.
	int ime_highlight_start; // For R_TEXT_EDIT: Which part of text needs to be highlighted.
	int ime_highlight_length;
} r_event;

typedef bool (r_callback_fn)(const r_event* event, r_callback* self);
struct r_callback
{
	r_event_type event;
	r_callback_fn* function;
	// Persistent data is stored in r_ui_list.
	void* data;
	int   size;

	// The label that will be used in the context menu to represent this R_CONTEXT callback.
	char* menu_label;
	// The hotkey that triggers the R_CONTEXT callback. It will also be displayed in the context menu.
	uint32_t hotkey;
	uint32_t hotkey_modifier;

	// Linked list
	r_interact* parent;
	r_callback* next;
};

typedef struct r_paged_buffer
{
	void*   base;
	int64_t committed;
	int64_t reserved;
	int64_t page_size;
} r_paged_buffer;

typedef struct r_hashmap
{
	int64_t* keys;
	uint8_t* values;
	int sizeof_value;
	int num_slots;
} r_hashmap;

struct r_ui_list
{
	r_context* context; // context = NULL indicates a free slot.
	void*      userptr;

	 // If non-zero this list is treated like a popup: it uses the coordinate system and event loop of the parent, but is rendered to it's own target.
	struct r_ui_list* ephemeral_parent;
	// Single linked list for iteration.
	// [List Idx 0][Child of List Idx 0][Child of Child of List Idx 0][List Idx 1][List Idx 2]
	struct r_ui_list* chain_prev;
	struct r_ui_list* chain_next;

	int generation;
	int parent_generation; // Compared to ephemeral_parent->generation to recognize when the parent is deleted.

	char* title;

	uint64_t target; // Usually it refers to a window, but can also be a render target or similar. Can be a simple index, or a pointer, or a handle depending on frontend.

	bool is_ephemeral:1; // Ephemeral lists are deleted if they weren't recreated/kept alive during a rebuild.
	bool is_masked:1; // Wheter to apply the root_area as scissor during rendering.
	bool is_hidden:1; // Can be used by tabs and popups.
	bool block_event_fallthrough:1; // If false, events get "bubbled up" to other r_ui_lists with the same target.
	bool block_accelerator_mode:1; // If true, Alt will not enter accelerator mode while focus is inside this UI list.
	bool needs_redraw:1;
	bool needs_ephemeral_sweep:1; // Is set to true when a ephemeral was marked to be deleted. This allows cleaning up ephemeral lists without regular list rebuilds.
	bool is_marked_as_unused:1;
	bool is_marked_as_to_delete:1;
	bool is_modal:1;
	bool is_viewport:1;
	bool is_tooltip:1;
	bool has_scrollbar:1;
	bool close_on_outside_click:1;
	bool use_transform_matrix:1;
	float content_transform[2][3];
	float scroll_x, scroll_y;

	// Area that is representitive of what space this UI list visually fills. Viewports get masked to this area. The X/Y coordinate is 0, 0 for windows as it's relative to their own coordinate system.
	rect root_area;
	// Determines min/max for scrollable viewports.
	rect document_area;
	uint64_t anchor_id;
	//rect  global_mapping; // Where the ui_list is being displayed in global screen coordinates.

	int frame_parity;
	uint32_t build_count;
	r_paged_buffer draw_buffers[2];
	r_drawable*    draw_items;
	r_drawable*    draw_items_last_frame;
	int            count_draw_items;
	int            count_draw_items_last_frame;

	r_paged_buffer interact_buffers[2];
	r_interact*    interact_items;
	r_interact*    interact_items_last_frame;
	int            count_interact_items;
	int            count_interact_last_frame;

	r_paged_buffer storage_buffers[2];
	uint8_t*       storage_buffer;
	uint8_t*       storage_buffer_last_frame;
	int            storage_buffer_cur_tail;
	int            storage_buffer_last_tail;

	r_hashmap id_hash[2];

	rect     drawn_area;
	uint64_t drawn_target;
	rect     damage;
};

typedef struct r_interface
{
	rect     (*calc_outer_rect)    (r_context* c, int style, rect* inner);
	rect     (*calc_inner_rect)    (r_context* c, int style, rect outer);
	uint64_t (*get_viewport_target)(r_ui_list* parent, rect relative_to);
	rect     (*measure_text)       (r_context* c, int style, const char* text, int len, int wrap_width);
	rect     (*measure_icon)       (r_context* c, int style, int icon, int* gap);
	int      (*break_text)         (r_context* c, int style, const char* text, int len, int width, bool force, int* w);
	void     (*get_layout)         (r_context* c, int style, int* size, int* spacing);
	void     (*set_clipboard)      (r_context* c, const char* text);
	char*    (*get_clipboard)      (r_context* c);
} r_interface;

struct r_context
{
	// Configuration
	r_interface interface;

	// If text input is received while the focused element doesn't handle text input, automatically focus this element.
	uint64_t text_auto_focus_idx;

	// State
	bool is_in_navigation_mode : 1; // Activated when pressing Tab if the focused interactable has traps_tab_focus disabled.
	bool is_in_accelerator_mode : 1; // Activated when pressing Alt unless the focused interactable has traps_alt_key or its UI list has block_accelerator_mode.

	// IDs
	uint64_t hover_id;
	int      hover_part;
	uint64_t focus_id;
	uint64_t pressed_id;        // Functionally captured element.
	int      pressed_part;
	uint64_t pressed_group_id;  // Capture group (see r_interact.press_group).
	int      pressed_mouse_button;

	float    time_since_hover;
	float    time_since_focus;
	float    time_since_pressed;

	// For modals: if the UI list currently in focus becomes invisible unwind the focus_stack to the next visible focused element.
	// Whenever the focus changes the current last item in the stack is checked, if it is part of the same UI list.
	// The stack is stored as circular buffer.
	uint64_t   focus_stack[16];
	int        focus_stack_start_idx;
	int        focus_stack_end_idx;

	// Dragging
	uint64_t dragged_id;
	float    time_since_drag_start;
	int      drag_mouse_button;
	float    drag_origin_x, drag_origin_y;

	// Mouse state
	int      mouse_buttons;
	uint32_t modifier_flags;
	float    mouse_x, mouse_y;
	uint64_t mouse_target;
	float    pressed_next_repeat;
	float    hover_next_repeat;
	float    time_since_click;
	int      click_count;
	uint64_t last_click_id;
	int      last_click_button;

	// UI lists.
	r_ui_list*     chain_start;
	r_ui_list*     chain_end;

	r_paged_buffer ui_list_buffer;
	r_ui_list*     ui_lists;
	int            count_ui_lists;

	r_paged_buffer ephemeral_buffer;
	r_ui_list*     ephemeral;
	int            count_ephemeral;

	r_hashmap ephemeral_id_map;

	uint64_t drawn_hover_id, drawn_pressed_id, drawn_focus_id;

	void* userptr;
};

typedef enum
{
	R_BOTTOM     = (1 << 0),
	R_V_CENTER   = (1 << 1),
	R_TOP        = (1 << 2),
	R_LEFT       = (1 << 3),
	R_H_CENTER   = (1 << 4),
	R_RIGHT      = (1 << 5),
	// 
	R_TOP_LEFT      = R_TOP    | R_LEFT,
	R_BOTTOM_LEFT   = R_BOTTOM | R_LEFT,
	R_TOP_RIGHT     = R_TOP    | R_RIGHT,
	R_BOTTOM_RIGHT  = R_BOTTOM | R_RIGHT,
	R_TOP_CENTER    = R_TOP    | R_H_CENTER,
	R_BOTTOM_CENTER = R_BOTTOM | R_H_CENTER,
	R_LEFT_CENTER   = R_LEFT   | R_V_CENTER,
	R_RIGHT_CENTER  = R_RIGHT  | R_V_CENTER,
	R_CENTER        = R_H_CENTER | R_V_CENTER,
} r_alignment;

rect rect_expand_pt(rect a, int x, int y);
rect rect_expand(rect a, rect b);
rect rect_intersect(rect a, rect b);

void r_init_context(r_context* r);
void r_free_context(r_context* r);
r_ui_list* r_create_ui_list(r_context* c, uint64_t target, uint64_t root_id, int width, int height);
void       r_destroy_ui_list(r_ui_list* list);
// Ephemerals are UI list attached to a rectangle with automatic lifetime managment.
// Note here: Salt the r parameter here with r_salt to allow for multiple popups on a single item.
r_ui_list* r_ephemeral_keep_alive(r_ui_list* parent, uint64_t id);
void       r_ephemeral_mark_to_delete(r_ui_list* parent, uint64_t id);

typedef enum r_popup_flags
{
	R_POPUP_TOOLTIP   = 1 << 0, // Only shows something: no input (the mouse goes to what's below), at most RCUT_EPHEMERAL_MAX_ELEMENTS elements.
	R_POPUP_AUTOCLOSE = 1 << 1, // Clicking outside of it (and its child popups) or pressing Escape closes it (close_on_outside_click). For menus.
} r_popup_flags;
r_ui_list* r_popup               (r_ui_list* parent, rect r, uint32_t flags, uint64_t salt);
r_ui_list* r_popup_keep_alive    (r_ui_list* parent, rect r, uint64_t salt);
void       r_popup_mark_to_delete(r_ui_list* parent, rect r, uint64_t salt);

r_ui_list* r_viewport(r_ui_list* parent, rect r);
r_ui_list* r_viewport_keep_alive(r_ui_list* parent, rect r);
void       r_viewport_mark_to_delete(r_ui_list* parent, rect r);

r_ui_list* r_scroll_begin(r_ui_list* parent, rect r);
r_ui_list* r_scroll_begin_ex(r_ui_list* parent, rect r, int frame_style);
void       r_scroll_end  (r_ui_list* parent, r_ui_list* viewport);
void       r_scroll_to   (r_ui_list* viewport, float x, float y);

rect r_list_get_content_rect(r_ui_list* list);
void r_list_get_transform(r_ui_list* l, int* origin_x, int* origin_y, rect* clip);
bool r_list_is_visible(r_ui_list* l);
bool r_context_sweep(r_context* c);
void r_list_clear(r_ui_list* list); // Swap last_frame and current frame lists
void r_list_reconcile(r_ui_list* list); // Reconcile interaction state with the previous frame.
void r_list_set_visibility(r_ui_list* list, bool visible);
void r_damage_id(r_context* c, uint64_t id);
void r_damage_clear(r_context* c);
void r_advance_time(r_context* r, float dt);

r_drawable* r_list_find_this_frame_drawable(r_ui_list* list, uint64_t id);
r_drawable* r_list_find_last_frame_drawable(r_ui_list* list, uint64_t id);
r_interact* r_list_find_this_frame_interact(r_ui_list* list, uint64_t id);
r_interact* r_list_find_last_frame_interact(r_ui_list* list, uint64_t id);

// Low level utility functions
bool r_dispatch    (r_interact* target, r_event_type event_type, r_event* event);
void r_change_focus(r_context* c, r_interact* new_focus);
bool r_is_pressed_visual(r_context* c, uint64_t id); // for rendering the pressed look
bool r_focus_wants_text(r_context* c);
r_cursor r_hover_cursor(r_context* c);
r_interact* r_find_interact(r_context* c, uint64_t id);
bool r_close_popups(r_context* c, bool only_topmost);
void r_list_print(r_ui_list* l);

// Low level input
uint64_t r_post_mouse_move (r_context* r, uint64_t target, float x, float y);
uint64_t r_post_mouse_down (r_context* r, uint64_t target, int mouse_button, float x, float y);
uint64_t r_post_mouse_up   (r_context* r, uint64_t target, int mouse_button, float x, float y);
uint64_t r_post_text       (r_context* r, uint64_t target, const char* str);
uint64_t r_post_text_edit  (r_context* r, uint64_t target, const char* str, int start, int length);
uint64_t r_post_key_input  (r_context* r, uint64_t target, int32_t keycode, int32_t scancode, uint32_t modifier_flags);
uint64_t r_post_mouse_wheel(r_context* r, uint64_t target, float x, float y, float scroll_x, float scroll_y);

rect r_salt(rect r, uint64_t salt);
void r_fill(r_alignment dir, rect target, rect** r, int num_rects); // The rectangles should be initialized with their minimum sizes before calling.
void r_min_size(rect *r, int w, int h);

void r_relative_to_fraction(rect target, rect* r, float x_fraction, float y_fraction);
void r_relative_to   (r_alignment corner, rect target, rect* r);

void r_align         (r_alignment corner, rect target, rect* r);
void r_align_group   (r_alignment corner, rect target, rect** r, int num_rects);
void r_align_fraction(rect target, rect* r, float x_fraction, float y_fraction);

rect r_aligned(r_alignment corner, rect target, rect src);
rect r_aligned_fraction(rect target, rect src, float x_fraction, float y_fraction);

void r_aside         (r_alignment corner, rect target, rect* r);
void r_aside_group   (r_alignment corner, rect target, rect** r, int num_rects);
void r_aside_fraction(rect target, rect* r, float x_fraction, float y_fraction);

rect r_cut_px      (r_alignment dir, rect* to_cut, int pixels,     uint64_t salt);
rect r_cut_fraction(r_alignment dir, rect* to_cut, float fraction, uint64_t salt);
void r_cut_group   (r_alignment dir, rect* to_cut, rect** r, int num_rects);

rect r_expand_px   (r_alignment dir, rect* to_expand, int pixels, uint64_t salt);
void r_expand_group(r_alignment dir, rect* to_expand, rect** r, int num_rects);

rect r_offset     (rect r, int x_offset, int y_offset);
rect r_grow       (rect r, int pixels);
rect r_grow_sides (rect r, int left, int top, int right, int bottom);

static inline rect r_shrink(rect r, int pixels){ return r_grow(r, -pixels); }
static inline rect r_shrink_sides(rect r, int left, int top, int right, int bottom){ return r_grow_sides(r, -left, -top, -right, -bottom); }

rect r_inner(r_context* c, int style, rect outer);
rect r_outer(r_context* c, int style, rect* inner);
rect r_with_margins(r_context* c, int style, rect outer);
int  r_layout_default_size   (r_context* c, int style);
int  r_layout_default_spacing(r_context* c, int style);
rect r_remove_margins(r_context* c, int style, rect with_margins);

rect r_measure(r_context* c, int style, const char* label);
rect r_measure_n(r_context* c, int style, const char* text, int len);
rect r_measure_wrapped(r_context* c, int style, const char* text, int wrap_width);
rect r_measure_ex(r_context* c, int style, int icon, const char* text);
int  r_break_text(r_context* c, int style, const char* text, int len, int width, bool force, int* w);
int  r_text_index_at(r_context* c, int style, const char* text, int x);

static inline void r_style_measure_all(r_context* c, int style, const char* label, rect* outer_with_margins, rect* outer, rect* inner)
{
	*inner = r_measure(c, style, label);
	*outer = r_outer(c, style, inner);
	if(outer_with_margins != NULL)
		*outer_with_margins = r_with_margins(c, style, *outer);
}

// Low level UI building
r_drawable* r_style               (r_ui_list* l, rect r, int style);
r_drawable* r_labeled             (r_ui_list* l, rect r, int style, const char* text);
r_callback* r_attach_callback     (r_ui_list* l, rect r, r_event_type event, r_callback_fn* function);
void        r_callback_set_data   (r_ui_list* l, r_callback* c, const void* data, int data_length);
void        r_callback_share_data (r_ui_list* l, r_callback* dst, r_callback* src);
void*       r_storage_alloc       (r_ui_list* l, int size); // Returns a pointer to a block of memory with lifetime scoped to this rebuild.
void*       r_store               (r_ui_list* l, const void* data, int size); // r_storage_alloc + copy, zeroed if data is NULL.
char*       r_store_string        (r_ui_list* l, const char* s);

bool        r_copy_last_frame_data(r_ui_list* l, r_callback* c);
void        r_set_press_group     (r_ui_list* l, rect r, uint64_t group_id);
r_interact* r_interactable        (r_ui_list* l, rect r);
r_interact* r_interactable_new    (r_ui_list* l, rect r);
r_callback* r_attach_callback_to  (r_ui_list* l, r_interact* it, r_event_type event, r_callback_fn* function);

// Data binding
typedef void (r_fn)(void);

#define R_SIGNATURE_LIST(X)  \
    X(b,     bool) \
    X(i32,   int32_t) \
    X(i64,   int64_t) \
    X(u32,   uint32_t) \
    X(u64,   uint64_t) \
    X(f32,   float) \
    X(f64,   double)

typedef enum r_signature
{
#define X(tag, ret) R_##tag ##_FN, R_##tag ##_FN_i32, R_##tag ##_FN_u64, R_##tag ##_FN_ptr, R_##tag ##_FN_ptr_i32,
    R_v_FN, R_v_FN_ptr,
    R_SIGNATURE_LIST(X)
#undef X
#define X(tag, target_type) R_v_FN_##tag, R_v_FN_i32_##tag, R_v_FN_u64_##tag, R_v_FN_ptr_##tag, R_v_FN_ptr_i32_##tag,
    R_SIGNATURE_LIST(X)
#undef X
    R_SIGNATURE_MAX
} r_signature;

typedef struct r_data_desc {
#define X(tag, ret) ret (*getter_##tag)(void); ret (*getter_##tag ##_i32)(int); ret (*getter_##tag ##_u64)(uint64_t); ret (*getter_##tag ##_ptr)(void*); ret (*getter_##tag ##_ptr_i32)(void*);
    R_SIGNATURE_LIST(X)
#undef X
#define X(tag, target_type) void (*setter_##tag)(target_type); void (*setter_i32_##tag)(int, target_type); void (*setter_u64_##tag)(uint64_t, target_type); void (*setter_ptr_##tag)(void*, target_type);  void (*setter_ptr_i32_##tag)(void*, int32_t, target_type);
    R_SIGNATURE_LIST(X)
#undef X
    uint64_t index;
    void* pointer;
} r_data_desc;

typedef enum r_data_type
{
	R_BOOL,  // True if any bit is set, false if none is set.
	R_FLOAT, // Only 32 bit (float) and 64 bit (double) supported.
	R_INT,   // Most significant bit is sign bit.
	R_UINT,  // Always positive.
} r_data_type;

typedef struct {
    bool use_function_indirection;
	union{
		struct{
            r_data_type ptr_type;
			void*       ptr;
			int     num_bits;
			int   bit_offset;
		};
		struct{
            r_fn* getter;
            r_fn* setter;
            r_signature setter_sig;
            r_signature getter_sig;
            uint64_t index;
            void* pointer;
		};
	};
} r_data_binding;

void    r_call(r_signature signature, r_fn* fn, uint64_t i, double d, void* ptr);

void    r_seti(r_data_binding binding, int64_t value);
int64_t r_geti(r_data_binding binding);

void    r_setd(r_data_binding binding, double value);
double  r_getd(r_data_binding binding);

typedef enum r_dynamic_rect_kind
{
	R_DYN_STATIC,  // No custom update.
	R_DYN_ALIGNED, // Align by fraction.
	R_DYN_FILLED,  // Stretch by fraction.
	R_DYN_TOGGLED, // Draw with state if toggled.
	R_DYN_CUSTOM,  // Custom update function.
} r_dynamic_rect_kind;

typedef struct r_dynamic_rect_data
{
	uint8_t kind;
	r_data_binding binding;
	double drawn;
	union{
		struct{ rect area; float min, max; r_alignment from; } aligned, filled;
		struct{ int64_t value; bool set_value; int state; int icon; } toggled;
		struct{ void (*update)(r_context* c, r_drawable* d); void* data; } custom;
	};
} r_dynamic_rect_data;

rect        r_dynamic_rect    (const r_dynamic_rect_data* y, rect r);
r_drawable  r_drawable_eval   (r_context* c, const r_drawable* d);
r_drawable* r_drawable_thumb  (r_ui_list* l, r_drawable* d, r_data_binding binding, float min, float max, rect area, r_alignment from);
r_drawable* r_drawable_fill   (r_ui_list* l, r_drawable* d, r_data_binding binding, float min, float max, rect area, r_alignment from);
r_drawable* r_drawable_toggled(r_ui_list* l, r_drawable* d, r_data_binding binding, int64_t value, bool set_value, int state, int icon);
r_drawable* r_drawable_custom (r_ui_list* l, r_drawable* d, r_data_binding binding, void (*update)(r_context* c, r_drawable* d), void* data);
bool        r_drawable_take_change(r_drawable* d, rect* damage);

#define R_TYPEOF(V) _Generic((V),                               \
        _Bool:              R_BOOL,                              \
        float:              R_FLOAT, double:             R_FLOAT,\
        char:               R_INT,                              \
        signed char:        R_INT,   short:              R_INT, \
        int:                R_INT,   long:               R_INT, \
        long long:          R_INT,                              \
        unsigned char:      R_UINT,  unsigned short:     R_UINT,\
        unsigned int:       R_UINT,  unsigned long:      R_UINT,\
        unsigned long long: R_UINT)

#define R_GETSET(...) r_data_compact((r_data_desc){ __VA_ARGS__ })
#define R_PTR(P) ((r_data_binding){                              \
    .ptr      = (void*)(P),                                      \
    .num_bits = (int)(sizeof(*(P)) * 8),                         \
    .ptr_type = R_TYPEOF(*(P)) })

#define R_BITFIELD(BASE, OFFSET, LENGTH) ((r_data_binding){      \
    .ptr        = (void*)(BASE),                                 \
    .num_bits   = (LENGTH),                                      \
    .bit_offset = (OFFSET),                                      \
    .ptr_type   = R_TYPEOF(*(BASE)) })

static inline r_data_binding r_data_compact(r_data_desc d) {
    r_data_binding b = {0};
    b.use_function_indirection = true;
    b.index = d.index;
    b.pointer = d.pointer;
#define X(tag, ret) \
    if(d.getter_##tag) { b.getter = (r_fn*)d.getter_##tag; b.getter_sig = R_##tag ##_FN; } \
    if(d.getter_##tag ##_i32) { b.getter = (r_fn*)d.getter_##tag ##_i32; b.getter_sig = R_##tag ##_FN ##_i32; } \
    if(d.getter_##tag ##_u64) { b.getter = (r_fn*)d.getter_##tag ##_u64; b.getter_sig = R_##tag ##_FN ##_u64; } \
    if(d.getter_##tag ##_ptr) { b.getter = (r_fn*)d.getter_##tag ##_ptr; b.getter_sig = R_##tag ##_FN ##_ptr; } \
    if(d.getter_##tag ##_ptr_i32) { b.getter = (r_fn*)d.getter_##tag ##_ptr_i32; b.getter_sig = R_##tag ##_FN ##_ptr_i32; }
    R_SIGNATURE_LIST(X)
#undef X
#define X(tag, type) \
    if(d.setter_##tag) { b.setter = (r_fn*)d.setter_##tag; b.setter_sig = R_v_FN_##tag; } \
    if(d.setter_i32_##tag) { b.setter = (r_fn*)d.setter_i32_##tag; b.setter_sig = R_v_FN_i32_##tag; } \
    if(d.setter_u64_##tag) { b.setter = (r_fn*)d.setter_u64_##tag; b.setter_sig = R_v_FN_u64_##tag; } \
    if(d.setter_ptr_##tag) { b.setter = (r_fn*)d.setter_ptr_##tag; b.setter_sig = R_v_FN_ptr_##tag; } \
    if(d.setter_ptr_i32_##tag) { b.setter = (r_fn*)d.setter_ptr_i32_##tag; b.setter_sig = R_v_FN_ptr_i32_##tag; }
    R_SIGNATURE_LIST(X)
#undef X
    return b;
}

// High level UI building
void r_button          (r_ui_list* l, rect r, const char* text);
r_drawable* r_button_ex(r_ui_list* l, rect r, int style, int icon, const char* text);
void r_label           (r_ui_list* l, rect r, const char* text);
r_drawable* r_label_ex(r_ui_list* l, rect r, int style, int icon, const char* text);
void r_label_value     (r_ui_list* l, rect r, const char* fmt, r_data_binding binding);
void r_icon            (r_ui_list* l, rect r, int style, int icon);
void r_header          (r_ui_list* l, rect r, const char* text);
void r_paragraph       (r_ui_list* l, rect r, int style, const char* text);
void r_panel           (r_ui_list* l, rect r, int style);
void r_separator       (r_ui_list* l, rect r);

void r_on_click        (r_ui_list* l, rect r, void (*on_click)(void));
void r_on_click_i32    (r_ui_list* l, rect r, void (*on_click)(int32_t), int32_t value);
void r_on_click_u64    (r_ui_list* l, rect r, void (*on_click)(uint64_t), uint64_t value);
void r_on_click_f32    (r_ui_list* l, rect r, void (*on_click)(float), float value);
void r_on_click_f64    (r_ui_list* l, rect r, void (*on_click)(double), double value);
void r_on_click_ptr    (r_ui_list* l, rect r, void (*on_click)(void*), void* value);
void r_on_click_ptr_i32(r_ui_list* l, rect r, void (*on_click)(void*, int32_t), void* ptr, int index);

void r_slider    (r_ui_list* l, rect r, float min, float max, float step, r_data_binding binding);
void r_slider_box(r_ui_list* l, rect r, const char* text, float min, float max, float step, r_data_binding binding);
void r_spinbox   (r_ui_list* l, rect r, float min, float max, float step, r_data_binding binding);
void r_progress  (r_ui_list* l, rect r, float fraction);

void r_checkbox  (r_ui_list* l, rect r, const char* text, r_data_binding binding);
void r_toggle    (r_ui_list* l, rect r, const char* text, r_data_binding binding);
void r_radio     (r_ui_list* l, rect r, const char* text, int64_t value, r_data_binding binding);
void r_selectable(r_ui_list* l, rect r, const char* text, int64_t value, r_data_binding binding);
void r_tabs      (r_ui_list* l, rect r, char** labels, int count, r_data_binding selected, r_ui_list** pages);
void r_dropdown  (r_ui_list* l, rect r, char** items, int count, r_data_binding selected);
void r_segmented (r_ui_list* l, rect r, char** labels, int count, r_data_binding selected);
bool r_tree_node (r_ui_list* l, rect r, int depth, const char* text, int icon, bool has_children, r_data_binding open, int64_t value, r_data_binding selected);
void r_split     (r_ui_list* l, rect r, r_alignment dir, r_data_binding fraction, int min_first, int min_second, rect* first, rect* second);

typedef struct r_text_edit_state
{
	int caret;
	int anchor;
	int scroll_x, scroll_y;
	int last_caret;
	int preferred_x;
	bool multiline;
	char*  buffer;
	int    capacity;
	int    style;
	char*  placeholder;
	rect   inner;
	void (*on_submit)(char* buffer);
} r_text_edit_state;
void r_line_edit (r_ui_list* l, rect r, char* buffer, int capacity, const char* placeholder);
void r_text_area (r_ui_list* l, rect r, char* buffer, int capacity, const char* placeholder);
void r_text_edit_ex(r_ui_list* l, rect r, char* buffer, int capacity, const char* placeholder, int style, bool multiline);
void r_line_edit_on_submit(r_ui_list* l, rect r, void (*on_submit)(char* buffer));

typedef struct r_menu_item
{
	char* label;
	int   icon;
	char* shortcut;
	void (*on_click)(void);
	const struct r_menu_item* submenu;
	bool* checked;
	bool  disabled;
} r_menu_item;
void       r_menubar      (r_ui_list* l, rect r, const r_menu_item* menus);
r_ui_list* r_menu_popup   (r_ui_list* parent, rect anchor, const r_menu_item* items, r_alignment side);
r_callback* r_on_context  (r_ui_list* l, rect r, const char* label, void (*on_select)(void));
rect       r_popup_place  (r_ui_list* parent, rect anchor, int w, int h, r_alignment side);

rect r_dialog_begin(r_ui_list* dialog);

void r_tooltip(r_ui_list* l, rect r, const char* text);

typedef struct r_layout_iter
{
	r_ui_list*  l;
	rect        start;
	rect        area;
	r_alignment dir;
	int         size;
	int         spacing;
	int         count;
	r_drawable* bg;
	int         style;
	int         line_h;
	int         line_first;
} r_layout_iter;

r_layout_iter r_layout        (r_ui_list* l, rect area, r_alignment dir, int style);
r_layout_iter r_stack         (r_ui_list* l, rect area, r_alignment dir);
static inline r_layout_iter r_layout_rows   (r_ui_list* l, rect area, int style){ return r_layout(l, area, R_TOP, style); }
static inline r_layout_iter r_layout_columns(r_ui_list* l, rect area, int style){ return r_layout(l, area, R_LEFT, style); }
rect          r_next          (r_layout_iter* s, int size);
rect          r_next_fraction (r_layout_iter* s, float fraction);
rect          r_next_fit      (r_layout_iter* s, int style, const char* text);
rect          r_next_fit_ex   (r_layout_iter* s, int style, int icon, const char* text);
rect          r_remaining          (r_layout_iter* s);
rect          r_next_keyed    (r_layout_iter* s, int size, uint64_t key);
void          r_gap           (r_layout_iter* s, int size);
r_layout_iter r_row           (r_layout_iter* s, int size, r_alignment dir);
r_layout_iter r_group_begin   (r_layout_iter* parent, int style);
rect          r_group_end     (r_layout_iter* group, r_layout_iter* parent);
//
r_layout_iter r_flow_begin    (r_layout_iter* parent, int style);
void          r_text          (r_layout_iter* flow, int style, const char* text);
rect          r_flow_end      (r_layout_iter* flow, r_layout_iter* parent);
