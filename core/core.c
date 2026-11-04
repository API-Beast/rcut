#include "internal.h"
#include <stddef.h>

#ifdef _WIN32
	typedef struct _SYSTEM_INFO {
		union {
			uint32_t dwOemId;
			struct {
				uint16_t wProcessorArchitecture;
				uint16_t wReserved;
			};
		};
		uint32_t  dwPageSize;
		void*     lpMinimumApplicationAddress;
		void*     lpMaximumApplicationAddress;
		uint32_t* dwActiveProcessorMask;
		uint32_t  dwNumberOfProcessors;
		uint32_t  dwProcessorType;
		uint32_t  dwAllocationGranularity;
		uint16_t  wProcessorLevel;
		uint16_t  wProcessorRevision;
	} SYSTEM_INFO;

	#define MEM_COMMIT   0x00001000UL
	#define MEM_RESERVE  0x00002000UL
	#define MEM_RELEASE  0x00008000UL
	#define PAGE_NOACCESS   0x01UL
	#define PAGE_READWRITE  0x04UL

	void*  __stdcall VirtualAlloc (void* lpAddress, size_t dwSize, uint32_t flAllocationType, uint32_t flProtect);
	int    __stdcall VirtualFree  (void* lpAddress, size_t dwSize, uint32_t  dwFreeType);
	void   __stdcall GetSystemInfo(SYSTEM_INFO* lpSystemInfo);
#else
  #include <sys/mman.h>
  #include <unistd.h>
#endif

static inline size_t get_page_size(void) {
#ifdef _WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwPageSize;
#else
	return (size_t)sysconf(_SC_PAGESIZE);
#endif
}

static inline size_t align_up(size_t v, size_t align)
{
	return (v + align - 1) & ~(align - 1);
}

static inline bool pbuffer_ensure(r_paged_buffer* a, size_t needed_bytes);

static inline bool pbuffer_init(r_paged_buffer* a, size_t reserve_bytes, size_t commit_bytes)
{
	a->page_size = get_page_size();
	reserve_bytes = align_up(reserve_bytes, a->page_size);

#ifdef _WIN32
	a->base = VirtualAlloc(NULL, reserve_bytes, MEM_RESERVE, PAGE_NOACCESS);
	if(!a->base) return false;
#else
	a->base = mmap(NULL, reserve_bytes, PROT_NONE,
				   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(a->base == MAP_FAILED) return false;
#endif

	a->reserved  = reserve_bytes;
	a->committed = 0;
	pbuffer_ensure(a, commit_bytes);
	return true;
}

// Ensure at least `needed_bytes` total are committed.
static inline bool pbuffer_ensure(r_paged_buffer* a, size_t needed_bytes)
{
	if(needed_bytes <= a->committed) return true;
	if(needed_bytes >  a->reserved)  return false;

	size_t new_committed = align_up(needed_bytes, a->page_size);
	void* region = (char*)a->base + a->committed;
	size_t size  = new_committed - a->committed;
#ifdef _WIN32
	if(!VirtualAlloc(region, size, MEM_COMMIT, PAGE_READWRITE)) return false;
#else
	if(mprotect(region, size, PROT_READ | PROT_WRITE) != 0) return false;
#endif

	a->committed = new_committed; 
	return true;
}

static inline void pbuffer_free(r_paged_buffer* a) {
	if(!a->base) return;
#ifdef _WIN32
	VirtualFree(a->base, 0, MEM_RELEASE);
#else
	munmap(a->base, a->reserved);
#endif
	*a = (r_paged_buffer){0};
}

static inline bool pbuffer_contains_ptr(r_paged_buffer* a, void* ptr)
{
	if(ptr < a->base) return false;
	if(ptr > (void*)((uint8_t*)a->base + a->committed)) return false;
	return true;
}

static uint64_t bits_load(uint8_t* p, int offset, int num_bits) {
    if(offset == 0 && num_bits == 32) return *(uint32_t*)p;
    if(offset == 0 && num_bits == 64) return *(uint64_t*)p;
    int lo = offset >> 3, hi = (offset + num_bits - 1) >> 3;
    uint64_t raw = 0;
    for (int i = hi; i >= lo; i--) raw = (raw << 8) | p[i];
    raw >>= offset & 7;
    return num_bits < 64 ? raw & (((uint64_t)1 << num_bits) - 1) : raw;
}

static void bits_store(uint8_t* p, int offset, int num_bits, uint64_t value) {
    if(offset == 0 && num_bits == 32) { *(uint32_t*)p = (uint32_t)value; return; }
    if(offset == 0 && num_bits == 64) { *(uint64_t*)p = value; return; }
    uint64_t mask = num_bits < 64 ? (((uint64_t)1 << num_bits) - 1) : ~(uint64_t)0;
    int lo = offset >> 3, hi = (offset + num_bits - 1) >> 3;
    uint64_t raw = 0;
    for (int i = hi; i >= lo; i--) raw = (raw << 8) | p[i];
    raw = (raw & ~(mask << (offset & 7))) | ((value & mask) << (offset & 7));
    for (int i = lo; i <= hi; i++) { p[i] = (uint8_t)raw; raw >>= 8; }
}

void r_call(r_signature signature, r_fn* fn, uint64_t i, double d, void* ptr) {
    switch (signature) {
    case R_v_FN:     ((void(*)(void)) fn)(); break;
    case R_v_FN_ptr: ((void(*)(void*))fn)(ptr); break;
#define X(tag, ret) \
    case R_##tag ##_FN:           ((ret(*)(void))fn)(); break; \
    case R_##tag ##_FN ##_i32:    ((ret(*)(int))fn)((int)i); break; \
    case R_##tag ##_FN ##_u64:    ((ret(*)(uint64_t))fn)(i); break; \
    case R_##tag ##_FN ##_ptr:    ((ret(*)(void*))fn)(ptr); break; \
    case R_##tag ##_FN ##_ptr_i32: ((ret(*)(void*, int))fn)(ptr, (int)i); break;
    R_SIGNATURE_LIST(X)
#undef X
#define X(tag, type) \
    case R_v_FN_##tag:         ((void(*)(type))fn)((type)_Generic((type){0}, float: d, double: d, default: i)); break; \
    case R_v_FN_i32_##tag:     ((void(*)(int, type))fn)((int)i, (type)_Generic((type){0}, float: d, double: d, default: i)); break; \
    case R_v_FN_u64_##tag:     ((void(*)(uint64_t, type))fn)(i, (type)_Generic((type){0}, float: d, double: d, default: i)); break; \
    case R_v_FN_ptr_##tag:     ((void(*)(void*, type))fn)(ptr, (type)_Generic((type){0}, float: d, double: d, default: i)); break; \
    case R_v_FN_ptr_i32_##tag: ((void(*)(void*, int32_t, type))fn)(ptr, (int32_t)i, (type)_Generic((type){0}, float: d, double: d, default: i)); break;
    R_SIGNATURE_LIST(X)
#undef X
    default: assert(false); return;
    }
}

int64_t r_geti(r_data_binding b) {
    if(b.use_function_indirection) {
        switch (b.getter_sig) {
#define X(tag, ret) \
        case R_##tag ##_FN:           return (int64_t)((ret(*)(void))       b.getter)(); \
        case R_##tag ##_FN ##_i32:    return (int64_t)((ret(*)(int))        b.getter)((int)b.index); \
        case R_##tag ##_FN ##_u64:    return (int64_t)((ret(*)(uint64_t))   b.getter)(b.index); \
        case R_##tag ##_FN ##_ptr:    return (int64_t)((ret(*)(void*))      b.getter)(b.pointer); \
        case R_##tag ##_FN ##_ptr_i32: return (int64_t)((ret(*)(void*, int)) b.getter)(b.pointer, (int)b.index);
        R_SIGNATURE_LIST(X)
#undef X
        default: assert(false); return 0;
        }
    }
    uint64_t raw = bits_load(b.ptr, b.bit_offset, b.num_bits);
    switch (b.ptr_type) {
    case R_BOOL: return raw != 0;
    case R_UINT: return (int64_t)raw;
    case R_INT: { int s = 64 - b.num_bits; return ((int64_t)(raw << s)) >> s; }
    case R_FLOAT:
        if(b.num_bits == 64) { double d; memcpy(&d, &raw, 8); return (int64_t)d; }
        else { uint32_t u = (uint32_t)raw; float f; memcpy(&f, &u, 4); return (int64_t)f; }
    }
    return 0;
}

double r_getd(r_data_binding b) {
    if(b.use_function_indirection) {
        switch (b.getter_sig) {
#define X(tag, ret) \
        case R_##tag ##_FN:           return (double)((ret(*)(void))       b.getter)(); \
        case R_##tag ##_FN ##_i32:    return (double)((ret(*)(int))        b.getter)((int)b.index); \
        case R_##tag ##_FN ##_u64:    return (double)((ret(*)(uint64_t))   b.getter)(b.index); \
        case R_##tag ##_FN ##_ptr:    return (double)((ret(*)(void*))      b.getter)(b.pointer); \
        case R_##tag ##_FN ##_ptr_i32: return (double)((ret(*)(void*, int)) b.getter)(b.pointer, (int)b.index);
        R_SIGNATURE_LIST(X)
#undef X
        default: assert(false); return 0;
        }
    }
    uint64_t raw = bits_load(b.ptr, b.bit_offset, b.num_bits);
    switch (b.ptr_type) {
    case R_BOOL: return raw != 0;
    case R_UINT: return (double)raw;
    case R_INT: { int s = 64 - b.num_bits; return (double)(((int64_t)(raw << s)) >> s); }
    case R_FLOAT:
        if(b.num_bits == 64) { double d; memcpy(&d, &raw, 8); return d; }
        else { uint32_t u = (uint32_t)raw; float f; memcpy(&f, &u, 4); return f; }
    }
    return 0;
}

void r_seti(r_data_binding b, int64_t value) {
    if(b.use_function_indirection) {
        switch (b.setter_sig) {
#define X(tag, type) \
        case R_v_FN_##tag:         ((void(*)(type))             b.setter)((type)value); return; \
        case R_v_FN_i32_##tag:     ((void(*)(int, type))        b.setter)((int)b.index, (type)value); return; \
        case R_v_FN_u64_##tag:     ((void(*)(uint64_t, type))   b.setter)(b.index, (type)value); return; \
        case R_v_FN_ptr_##tag:     ((void(*)(void*, type))      b.setter)(b.pointer, (type)value); return; \
        case R_v_FN_ptr_i32_##tag: ((void(*)(void*, int32_t, type)) b.setter)(b.pointer, (int32_t)b.index, (type)value); return;
        R_SIGNATURE_LIST(X)
#undef X
        default: assert(false); return;
        }
    }
    uint64_t raw;
    switch (b.ptr_type) {
    case R_BOOL: raw = value != 0; break;
    case R_FLOAT:
        if(b.num_bits == 64) { double d = (double)value; memcpy(&raw, &d, 8); }
        else { float f = (float)value; uint32_t u; memcpy(&u, &f, 4); raw = u; }
        break;
    default: raw = (uint64_t)value; break;
    }
    bits_store(b.ptr, b.bit_offset, b.num_bits, raw);
}

void r_setd(r_data_binding b, double value) {
    if(b.use_function_indirection) {
        switch (b.setter_sig) {
#define X(tag, type) \
        case R_v_FN_##tag:         ((void(*)(type))             b.setter)((type)value); return; \
        case R_v_FN_i32_##tag:     ((void(*)(int, type))        b.setter)((int)b.index, (type)value); return; \
        case R_v_FN_u64_##tag:     ((void(*)(uint64_t, type))   b.setter)(b.index, (type)value); return; \
        case R_v_FN_ptr_##tag:     ((void(*)(void*, type))      b.setter)(b.pointer, (type)value); return; \
        case R_v_FN_ptr_i32_##tag: ((void(*)(void*, int32_t, type)) b.setter)(b.pointer, (int32_t)b.index, (type)value); return;
        R_SIGNATURE_LIST(X)
#undef X
        default: assert(false); return;
        }
    }
    uint64_t raw;
    switch (b.ptr_type) {
    case R_BOOL: raw = value != 0; break;
    case R_FLOAT:
        if(b.num_bits == 64) { memcpy(&raw, &value, 8); }
        else { float f = (float)value; uint32_t u; memcpy(&u, &f, 4); raw = u; }
        break;
    case R_INT: raw = (uint64_t)(int64_t)value; break;
    default: raw = (uint64_t)value; break;
    }
    bits_store(b.ptr, b.bit_offset, b.num_bits, raw);
}

static r_drawable* push_drawable(r_ui_list* l)
{
	static r_drawable scratch;
	int p = l->frame_parity;
	bool success = pbuffer_ensure(&l->draw_buffers[p], (size_t)(l->count_draw_items + 1) * sizeof(r_drawable));
	assert(success);

	l->draw_items = l->draw_buffers[p].base;
	r_drawable* d = &l->draw_items[l->count_draw_items++];
	*d = (r_drawable){0};
	return d;
}

static r_interact* get_or_create_interactable(r_ui_list* l, rect r)
{
	int p = l->frame_parity;
	int* idx_ptr = hash_get(&l->id_hash[p], r.id);
	
	if(idx_ptr != NULL)
		return &l->interact_items[*idx_ptr];

	bool success = pbuffer_ensure(&l->interact_buffers[p], (size_t)(l->count_interact_items + 2) * sizeof(r_interact));
	assert(success); 

	l->interact_items = l->interact_buffers[p].base;
	int new_idx = l->count_interact_items++;
	r_interact* it = &l->interact_items[new_idx];
	*it = (r_interact){0};
	it->owner = l;
	it->r = r;
	it[1].r.id = 0;

	*(int*)hash_insert(&l->id_hash[p], r.id) = new_idx;
	return it;
}

void* r_storage_alloc(r_ui_list* l, int size)
{
	int p = l->frame_parity;
	size = (size + 7) & ~7;
	bool success = pbuffer_ensure(&l->storage_buffers[p], (size_t)l->storage_buffer_cur_tail + size);
	assert(success);

	l->storage_buffer = l->storage_buffers[p].base;
	void* ptr = l->storage_buffer + l->storage_buffer_cur_tail;
	l->storage_buffer_cur_tail += size;
	return ptr;
}

void* r_store(r_ui_list* l, const void* data, int size)
{
	void* p = r_storage_alloc(l, size);
	return data ? memcpy(p, data, size) : memset(p, 0, size);
}

char* r_store_string(r_ui_list* l, const char* s)
{
	return r_store(l, s, (int)strlen(s) + 1);
}

void r_init_context(r_context* c)
{
	*c = (r_context){0};
	pbuffer_init(&c->ui_list_buffer, sizeof(r_ui_list) * RCUT_CONTEXT_MAX_LISTS, sizeof(r_ui_list) * 16);
	c->ui_lists = c->ui_list_buffer.base;
	pbuffer_init(&c->ephemeral_buffer, sizeof(r_ui_list) * RCUT_CONTEXT_MAX_EPHEMERAL_LISTS, sizeof(r_ui_list) * 16);
	c->ephemeral = c->ephemeral_buffer.base;
	hash_init(&c->ephemeral_id_map, 256, sizeof(r_ui_list*));
	c->hover_next_repeat = RCUT_MOUSE_HOVER_INITIAL_DELAY;
}

static void free_list_buffers(r_ui_list* l)
{
	for(int i = 0; i < 2; i++)
	{
		if(l->draw_buffers[i].base == NULL) continue;
		pbuffer_free(&l->draw_buffers[i]);
		pbuffer_free(&l->interact_buffers[i]);
		pbuffer_free(&l->storage_buffers[i]);
		free(l->id_hash[i].keys); free(l->id_hash[i].values);
		l->id_hash[i] = (r_hashmap){0};
	}
}

void r_free_context(r_context* c)
{
	for(int i = 0; i < c->count_ui_lists; i++) free_list_buffers(&c->ui_lists[i]);
	for(int i = 0; i < c->count_ephemeral; i++) free_list_buffers(&c->ephemeral[i]);
	pbuffer_free(&c->ui_list_buffer);
	pbuffer_free(&c->ephemeral_buffer);
	free(c->ephemeral_id_map.keys); free(c->ephemeral_id_map.values);
	*c = (r_context){0};
}

static void init_ui_list(r_ui_list* l, int max_draw, int max_interact)
{
	l->content_transform[0][0] = 1.0f;
	l->content_transform[1][1] = 1.0f;

	for(int i = 0; i < 2; i++)
	{
		if(l->draw_buffers[i].base != NULL &&
		   (l->draw_buffers[i].reserved     < (int64_t)(sizeof(r_drawable) * max_draw) ||
		    l->interact_buffers[i].reserved < (int64_t)(sizeof(r_interact) * (max_interact + 1))))
		{
			pbuffer_free(&l->draw_buffers[i]);
			pbuffer_free(&l->interact_buffers[i]);
			pbuffer_free(&l->storage_buffers[i]);
			free(l->id_hash[i].keys); free(l->id_hash[i].values);
		}
		if(l->draw_buffers[i].base == NULL)
		{
			pbuffer_init(&l->draw_buffers[i],     sizeof(r_drawable) * max_draw,           sizeof(r_drawable) * 16);
			pbuffer_init(&l->interact_buffers[i], sizeof(r_interact) * (max_interact + 1), sizeof(r_interact) * 16);
			pbuffer_init(&l->storage_buffers[i],  max_draw * 32 * 4, 64);
			hash_init(&l->id_hash[i], 256, sizeof(int));
		}
		else hash_clear(&l->id_hash[i]);
	}
	l->frame_parity = 0;
	l->draw_items                = l->draw_buffers[0].base;
	l->draw_items_last_frame     = l->draw_buffers[1].base;
	l->interact_items            = l->interact_buffers[0].base;
	l->interact_items_last_frame = l->interact_buffers[1].base;
	l->storage_buffer            = l->storage_buffers[0].base;
	l->storage_buffer_last_frame = l->storage_buffers[1].base;
}

static inline bool is_popup(r_ui_list* l)
{
	return l->ephemeral_parent && !l->is_viewport;
}

static void chain_insert_after(r_context* c, r_ui_list* prev, r_ui_list* new)
{
	if(prev == NULL)
	{
		new->chain_next = c->chain_start;
		if(c->chain_start) c->chain_start->chain_prev = new; else c->chain_end = new;
		c->chain_start = new;
		return;
	}
	new->chain_next = prev->chain_next;
	new->chain_prev = prev;
	if(prev->chain_next) prev->chain_next->chain_prev = new; else c->chain_end = new;
	prev->chain_next = new;
}

static void chain_remove(r_context* c, r_ui_list* l)
{
	if(l->chain_prev) l->chain_prev->chain_next = l->chain_next; else c->chain_start = l->chain_next;
	if(l->chain_next) l->chain_next->chain_prev = l->chain_prev; else c->chain_end = l->chain_prev;
	l->chain_prev = l->chain_next = NULL;
}

static void insert_into_chain(r_context* c, r_ui_list* new)
{
	if(is_popup(new)) { chain_insert_after(c, c->chain_end, new); return; }

	r_ui_list* parent = new->ephemeral_parent;
	r_ui_list* prev = parent;
	while(prev->chain_next != NULL && !is_popup(prev->chain_next) && is_descendant(prev->chain_next, parent))
		prev = prev->chain_next;
	chain_insert_after(c, prev, new);
}

r_ui_list* r_create_ui_list(r_context* c, uint64_t target, uint64_t root_id, int width, int height)
{
	return r_create_ui_list_ex(c, target, root_id, width, height, RCUT_LIST_MAX_ELEMENTS, RCUT_LIST_MAX_ELEMENTS);
}

r_ui_list* r_create_ui_list_ex(r_context* c, uint64_t target, uint64_t root_id, int width, int height, int max_drawables, int max_interactables)
{
	r_ui_list* l = NULL;

	for(int i = 0; i < c->count_ui_lists && !l; i++)
	if(c->ui_lists[i].context == NULL)
		l = &c->ui_lists[i];

	if(!l) l = c->ui_lists + c->count_ui_lists++;
	r_ui_list keep = *l;
	memset(l, 0, sizeof(r_ui_list));
	l->generation   = keep.generation;
	l->drawn_area   = keep.drawn_area;
	l->drawn_target = keep.drawn_target;

	init_ui_list(l, max_drawables, max_interactables);
	l->target = target;
	l->context = c;
	l->root_area = (rect){0, 0, width, height, root_id};
	l->resized = true;
	l->document_area = l->root_area;
	l->block_event_fallthrough = true;
	l->needs_redraw = true;

	bool success = pbuffer_ensure(&c->ui_list_buffer, (size_t)(c->count_ui_lists + 1) * sizeof(r_ui_list));
	assert(success);
	chain_insert_after(c, c->chain_end, l);

	return l;
}

static void destroy_root(r_context* c, r_ui_list* l)
{
	chain_remove(c, l);
	free_list_buffers(l);
	l->context = NULL;
	l->generation++;
}

void r_destroy_ui_list(r_ui_list* l)
{
	r_context* c = l->context;
	if(!c) return;
	if(l->ephemeral_parent) { r_ephemeral_mark_to_delete(l->ephemeral_parent, l->root_area.id); return; }
	l->is_marked_as_to_delete = true;
	r_list_set_visibility(l, false);
}

static r_ui_list* create_ephemeral(r_ui_list* parent, rect root, uint64_t target, bool viewport, int capacity)
{
	r_context* c = parent->context;
	r_ui_list* l = NULL;

	for(int i = 0; i < c->count_ephemeral && !l; i++)
	if(c->ephemeral[i].context == NULL)
		l = &c->ephemeral[i];

	if(!l)
	{
		bool success = pbuffer_ensure(&c->ephemeral_buffer, (size_t)(c->count_ephemeral + 1) * sizeof(r_ui_list));
		assert(success);

		l = c->ephemeral + c->count_ephemeral++;
		memset(l, 0, sizeof(r_ui_list));
	}

	r_ui_list keep = *l;
	memset(l, 0, sizeof(r_ui_list));
	memcpy(l->draw_buffers, keep.draw_buffers, sizeof(keep.draw_buffers));
	memcpy(l->interact_buffers, keep.interact_buffers, sizeof(keep.interact_buffers));
	memcpy(l->storage_buffers, keep.storage_buffers, sizeof(keep.storage_buffers));
	memcpy(l->id_hash, keep.id_hash, sizeof(keep.id_hash));
	l->generation   = keep.generation;
	l->drawn_area   = keep.drawn_area;
	l->drawn_target = keep.drawn_target;

	init_ui_list(l, capacity, capacity);
	l->ephemeral_parent  = parent;
	l->parent_generation = parent->generation;
	l->is_ephemeral = true;
	l->is_viewport  = viewport;
	l->context = c;
	l->root_area = root;
	l->target = target;
	l->block_event_fallthrough = false;
	l->needs_redraw = true;

	insert_into_chain(c, l);
	*hash_insertv(&c->ephemeral_id_map, root.id) = l;
	return l;
}

static void delete_ephemeral(r_context* c, r_ui_list* l)
{
	chain_remove(c, l);
	hash_remove(&c->ephemeral_id_map, l->root_area.id);
	l->context = NULL;
	l->generation++;
}

bool r_context_sweep(r_context* c)
{
	bool any = false, again = true;
	while(again)
	{
		again = false;
		for(int i = 0; i < c->count_ui_lists; i++)
		{
			r_ui_list* l = &c->ui_lists[i];
			if(l->context && l->is_marked_as_to_delete)
			{
				destroy_root(c, l);
				any = again = true;
			}
		}
		for(int i = 0; i < c->count_ephemeral; i++)
		{
			r_ui_list* l = &c->ephemeral[i];
			if(l->context == NULL) continue;
			r_ui_list* p = l->ephemeral_parent;
			bool parent_dead = p->context == NULL || p->generation != l->parent_generation;
			if(parent_dead || l->is_marked_as_to_delete || l->is_marked_as_unused)
			{
				delete_ephemeral(c, l);
				any = again = true;
			}
		}
	}
	for(r_ui_list* l = c->chain_start; l; l = l->chain_next)
		l->needs_ephemeral_sweep = false;
	return any;
}

r_ui_list* r_ephemeral_keep_alive(r_ui_list* parent, uint64_t id)
{
	r_context* c = parent->context;
	r_ui_list* result = hash_getv(&c->ephemeral_id_map, id);
	if(result == NULL || result->is_marked_as_to_delete) return NULL;
	result->is_marked_as_unused = false;
	return result;
}

void r_ephemeral_mark_to_delete(r_ui_list* parent, uint64_t id)
{
	r_context* c = parent->context;
	r_ui_list* result = hash_getv(&c->ephemeral_id_map, id);
	if(result != NULL)
	{
		result->is_marked_as_to_delete = true;
		r_list_set_visibility(result, false);
		r_ui_list* p = result->ephemeral_parent;
		while(p != NULL)
		{
			p->needs_ephemeral_sweep = true;
			p = p->ephemeral_parent;
		}
		return;
	}
}

r_ui_list* r_popup(r_ui_list* parent, rect root, uint32_t flags, uint64_t salt)
{
	bool tooltip = flags & R_POPUP_TOOLTIP;
	r_context* c = parent->context;
	uint64_t anchor = root.id;
	root = r_salt(root, salt);
	r_ui_list* result = hash_getv(&c->ephemeral_id_map, root.id);
	if(result != NULL)
	{
		if(result->is_marked_as_to_delete)
		{
			result->is_marked_as_to_delete = false;
			result->is_hidden = false;
			r_list_clear(result);
		}
		result->is_marked_as_unused = false;
		return result;
	}

	uint64_t target = parent->target;
	if(c->interface.get_viewport_target)
		target = c->interface.get_viewport_target(parent, root);

	result = create_ephemeral(parent, root, target, false, tooltip ? RCUT_EPHEMERAL_MAX_ELEMENTS : RCUT_LIST_MAX_ELEMENTS);
	result->anchor_id = anchor;
	result->is_tooltip = tooltip;
	result->close_on_outside_click = (flags & R_POPUP_AUTOCLOSE) != 0;
	return result;
}

r_ui_list* r_viewport(r_ui_list* parent, rect target)
{
	r_context* c = parent->context;
	r_ui_list* result = hash_getv(&c->ephemeral_id_map, target.id);
	if(result == NULL)
	{
		result = create_ephemeral(parent, target, parent->target, true, RCUT_LIST_MAX_ELEMENTS);
		result->anchor_id = target.id;
		result->is_masked = true;
	}

	result->is_marked_as_unused = false;
	result->is_marked_as_to_delete = false;
	result->root_area = target;
	r_list_clear(result);
	result->document_area = (rect){0, 0, target.w, 0, target.id};
	return result;
}

r_ui_list* r_popup_keep_alive(r_ui_list* parent, rect r, uint64_t salt)   { r = r_salt(r, salt); return r_ephemeral_keep_alive    (parent, r.id); }
void       r_popup_mark_to_delete(r_ui_list* parent, rect r, uint64_t salt){ r = r_salt(r, salt);       r_ephemeral_mark_to_delete(parent, r.id); }
r_ui_list* r_viewport_keep_alive(r_ui_list* parent, rect r){ return r_ephemeral_keep_alive    (parent, r.id); }
void       r_viewport_mark_to_delete(r_ui_list* parent, rect r){    r_ephemeral_mark_to_delete(parent, r.id); }

void r_list_clear(r_ui_list* l)
{
	int np = l->frame_parity ^ 1;
	l->frame_parity = np;
	l->resized = false;

	l->draw_items_last_frame       = l->draw_buffers[np ^ 1].base;
	l->count_draw_items_last_frame = l->count_draw_items;
	l->draw_items                  = l->draw_buffers[np].base;
	l->count_draw_items            = 0;

	l->interact_items_last_frame   = l->interact_buffers[np ^ 1].base;
	l->count_interact_last_frame   = l->count_interact_items;
	l->interact_items              = l->interact_buffers[np].base;
	l->count_interact_items        = 0;

	l->storage_buffer_last_frame   = l->storage_buffers[np ^ 1].base;
	l->storage_buffer_last_tail    = l->storage_buffer_cur_tail;
	l->storage_buffer              = l->storage_buffers[np].base;
	l->storage_buffer_cur_tail     = 0;

	l->needs_redraw = true;
	l->build_count++;

	hash_clear(&l->id_hash[np]);

	r_context* c = l->context;

	for(int i = 0; i < c->count_ephemeral; i++)
	if(c->ephemeral[i].context && c->ephemeral[i].ephemeral_parent == l)
	{
		c->ephemeral[i].is_marked_as_unused = true;
		l->needs_ephemeral_sweep = true;
	}
}

rect r_list_get_content_rect(r_ui_list* l)
{
	rect result = (rect){0};
	
	if(l->count_draw_items >= 1)
		result = l->draw_items[0].r;

	for(int i = 1; i < l->count_draw_items; i++)
		result = rect_expand(result, l->draw_items[i].r);
		
	return result;
}

void r_list_get_transform(r_ui_list* l, int* ox, int* oy, rect* clip)
{
	if(!l->ephemeral_parent)
	{
		*ox = l->root_area.x - (int)l->scroll_x;
		*oy = l->root_area.y - (int)l->scroll_y;
		*clip = l->root_area;
		return;
	}

	int px, py; rect pclip;
	r_list_get_transform(l->ephemeral_parent, &px, &py, &pclip);
	
	if(l->is_viewport)
	{
		rect area = r_offset(l->root_area, px, py);
		*clip = l->is_masked ? rect_intersect(area, pclip) : pclip;
		*ox = area.x - (int)l->scroll_x;
		*oy = area.y - (int)l->scroll_y;
	}
	else
	{
		*ox = px; *oy = py;
		*clip = l->is_masked ? r_offset(l->root_area, px, py) : (rect){ -(1 << 22), -(1 << 22), 1 << 23, 1 << 23 };
	}
}

bool r_list_is_visible(r_ui_list* l)
{
	for(; l; l = l->ephemeral_parent)
	if(l->is_hidden)
		return false;

	return true;
}

void r_damage_id(r_context* c, uint64_t id)
{
	if(!id) return;
	for(r_ui_list* l = c->chain_start; l; l = l->chain_next)
	for(int i = 0; i < l->count_draw_items; i++)
	{
		r_drawable* d = &l->draw_items[i];
		if((d->parent_id ? d->parent_id : d->r.id) != id) continue;
		rect r = r_drawable_eval(c, d).r;
		if(r.w <= 0 || r.h <= 0) continue;
		l->damage = l->damage.w > 0 ? rect_expand(l->damage, r) : r;
	}
}

void r_list_set_visibility(r_ui_list* l, bool visible)
{
	if(l->is_hidden == !visible) return; // no change
	l->is_hidden = !visible;
	l->needs_redraw = true;
	r_list_reconcile(l);
}

r_drawable* r_style(r_ui_list* l, rect r, int style)
{
	r_drawable* d = push_drawable(l);
	d->r = r; d->style = style; d->text = NULL;
	return d;
}

r_drawable* r_labeled(r_ui_list* l, rect outer, int style, const char* text)
{
	r_drawable* d = r_style(l, outer, style);
	r_drawable* label = r_style(l, r_inner(l->context, style, outer), R_STYLE_TEXT);
	label->text = (char*)text; label->outer_style = style;
	label->parent_id = outer.id;
	return d;
}

r_interact* r_interactable(r_ui_list* l, rect r)
{
	return get_or_create_interactable(l, r);
}

r_interact* r_interactable_new(r_ui_list* l, rect r)
{
	int p = l->frame_parity;
	if(!hash_get(&l->id_hash[p], r.id)) return get_or_create_interactable(l, r);
	// Note that successive

	bool success = pbuffer_ensure(&l->interact_buffers[p], (size_t)(l->count_interact_items + 2) * sizeof(r_interact));
	assert(success);

	l->interact_items = l->interact_buffers[p].base;
	r_interact* it = &l->interact_items[l->count_interact_items++];
	*it = (r_interact){ .owner = l, .r = r };
	it[1].r.id = 0;
	return it;
}

r_callback* r_attach_callback(r_ui_list* l, rect r, r_event_type event, r_callback_fn* function)
{
	return r_attach_callback_to(l, get_or_create_interactable(l, r), event, function);
}

r_callback* r_attach_callback_to(r_ui_list* l, r_interact* it, r_event_type event, r_callback_fn* function)
{
	r_callback* cb = r_storage_alloc(l, sizeof(r_callback));
	*cb = (r_callback){0};
	cb->event = event; cb->function = function;
	cb->parent = it;
	r_callback** p = &it->callbacks;
	while(*p) p = &(*p)->next;
	*p = cb;
	return cb;
}

void r_callback_set_data(r_ui_list* l, r_callback* c, const void* data, int data_length)
{
	c->data = r_store(l, data, data_length); c->size = data_length;
}

void r_callback_share_data(r_ui_list* l, r_callback* dst, r_callback* src)
{
	if(src->data)
	{
		// Don't share data from last frames callback. Use r_copy_last_frame_data instead.
		assert(pbuffer_contains_ptr(&l->storage_buffers[l->frame_parity], src->data));
		dst->data = src->data; dst->size = src->size;
	}
}

static float dynamic_fraction(const r_dynamic_rect_data* y)
{
	float min = y->aligned.min, max = y->aligned.max;
	return max != min ? clampf(((float)r_getd(y->binding) - min) / (max - min), 0.0f, 1.0f) : 0.0f;
}

rect r_dynamic_rect(const r_dynamic_rect_data* y, rect r)
{
	if(!y) return r;
	switch(y->kind)
	{
		case R_DYN_ALIGNED:
		{
			rect a = y->aligned.area;
			float t = dynamic_fraction(y);
			if(y->aligned.from & (R_RIGHT | R_BOTTOM)) t = 1.0f - t;
			if(y->aligned.from & (R_LEFT | R_RIGHT)) r.x = (int)(a.x + (a.w - r.w) * t);
			else                                     r.y = (int)(a.y + (a.h - r.h) * t);
			break;
		}
		case R_DYN_FILLED:
		{
			rect a = y->filled.area;
			float t = dynamic_fraction(y);
			int w = (int)(a.w * t), h = (int)(a.h * t);
			if(y->filled.from & R_RIGHT)  a.x += a.w - w;
			if(y->filled.from & R_BOTTOM) a.y += a.h - h;
			if(y->filled.from & (R_LEFT | R_RIGHT)) a.w = w; else a.h = h;
			r = (rect){ a.x, a.y, a.w, a.h, r.id };
			break;
		}
	}
	return r;
}

r_drawable r_drawable_eval(r_context* c, const r_drawable* d)
{
	r_drawable out = *d;
	const r_dynamic_rect_data* y = d->dynamic;
	if(!y) return out;
	out.r = r_dynamic_rect(y, d->r);
	switch(y->kind)
	{
		case R_DYN_TOGGLED:
		{
			int64_t v = r_geti(y->binding);
			bool on = y->toggled.set_value ? v == y->toggled.value : v != 0;
			if(on) out.style |= y->toggled.state;
			if(y->toggled.icon) out.icon = on ? y->toggled.icon : R_ICON_NONE;
			break;
		}
		case R_DYN_CUSTOM: y->custom.update(c, &out); break;
	}
	return out;
}

static bool is_bound(r_data_binding b)
{
	return b.use_function_indirection ? b.getter != NULL : b.ptr != NULL;
}

static r_drawable* set_dynamic(r_ui_list* l, r_drawable* d, r_dynamic_rect_data y)
{
	if(is_bound(y.binding)) y.drawn = r_getd(y.binding);
	d->dynamic = r_storage_alloc(l, sizeof(y));
	*d->dynamic = y;
	return d;
}

r_drawable* r_drawable_thumb(r_ui_list* l, r_drawable* d, r_data_binding binding, float min, float max, rect area, r_alignment from)
{
	return set_dynamic(l, d, (r_dynamic_rect_data){ R_DYN_ALIGNED, binding, .aligned = { area, min, max, from } });
}

r_drawable* r_drawable_fill(r_ui_list* l, r_drawable* d, r_data_binding binding, float min, float max, rect area, r_alignment from)
{
	return set_dynamic(l, d, (r_dynamic_rect_data){ R_DYN_FILLED, binding, .filled = { area, min, max, from } });
}

r_drawable* r_drawable_toggled(r_ui_list* l, r_drawable* d, r_data_binding binding, int64_t value, bool set_value, int state, int icon)
{
	return set_dynamic(l, d, (r_dynamic_rect_data){ R_DYN_TOGGLED, binding, .toggled = { value, set_value, state, icon } });
}

r_drawable* r_drawable_custom(r_ui_list* l, r_drawable* d, r_data_binding binding, void (*update)(r_context* c, r_drawable* d), void* data)
{
	return set_dynamic(l, d, (r_dynamic_rect_data){ R_DYN_CUSTOM, binding, .custom = { update, data } });
}

bool r_drawable_take_change(r_drawable* d, rect* damage)
{
	r_dynamic_rect_data* y = d->dynamic;
	if(!y || !is_bound(y->binding)) return false;
	double v = r_getd(y->binding);
	if(v == y->drawn) return false;
	y->drawn = v;
	*damage = y->kind == R_DYN_ALIGNED || y->kind == R_DYN_FILLED ? rect_expand(y->aligned.area, d->r) : d->r;
	return true;
}

static int count_parts(r_interact* items, int count, int first)
{
	int n = 1;
	while(first + n < count && items[first + n].r.id == items[first].r.id) n++;
	return n;
}

bool r_copy_last_frame_data(r_ui_list* l, r_callback* c)
{
	if(!c->parent) return false;
	uint64_t id = c->parent->r.id;
	int* this_idx = hash_get(&l->id_hash[l->frame_parity], id);
	int* last_idx = hash_get(&l->id_hash[l->frame_parity ^ 1], id);
	if(!this_idx || !last_idx) return false;
	int part = (int)(c->parent - &l->interact_items[*this_idx]);
	if(part < 0 || part >= count_parts(l->interact_items_last_frame, l->count_interact_last_frame, *last_idx)) return false;

	r_interact* old = &l->interact_items_last_frame[*last_idx + part];
	for(r_callback* other = old->callbacks; other; other = other->next)
	{
		if(other->event == c->event && other->data != NULL)
		{
			r_callback_set_data(l, c, other->data, other->size);
			return true;
		}
	}
	return false;
}

void r_set_press_group(r_ui_list* l, rect r, uint64_t group_id)
{
	get_or_create_interactable(l, r)->press_group = group_id;
}

r_interact* r_list_find_this_frame_interact(r_ui_list* l, uint64_t id)
{
	int* idx = hash_get(&l->id_hash[l->frame_parity], id);
	return idx ? &l->interact_items[*idx] : NULL;
}

r_interact* r_list_find_last_frame_interact(r_ui_list* l, uint64_t id)
{
	int* idx = hash_get(&l->id_hash[l->frame_parity ^ 1], id);
	return idx ? &l->interact_items_last_frame[*idx] : NULL;
}

r_drawable* r_list_find_this_frame_drawable(r_ui_list* l, uint64_t id)
{
	for(int i = 0; i < l->count_draw_items; i++)
	if(l->draw_items[i].r.id == id)
		return &l->draw_items[i];

	return NULL;
}

r_drawable* r_list_find_last_frame_drawable(r_ui_list* l, uint64_t id)
{
	for(int i = 0; i < l->count_draw_items_last_frame; i++)
	if(l->draw_items_last_frame[i].r.id == id)
		return &l->draw_items_last_frame[i];

	return NULL;
}
