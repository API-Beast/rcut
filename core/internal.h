#ifndef RCUT_CORE_INTERNAL_H
#define RCUT_CORE_INTERNAL_H
#include "../rcut_core.h"
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define STUB {static bool wrote_warning = false; if(!wrote_warning){ wrote_warning = true; fprintf(stderr, "STUB: %s uses a dumb placeholder implementation.\n", __func__); }}

#define MAX(a, b) (((a) < (b)) ? (b) : (a))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define TAU 6.28318530717958647692f // Full turn in radians
#define COUNTOF(a) ((int)(sizeof(a) / sizeof((a)[0])))

static inline uint32_t xxhash32(uint32_t x, uint32_t y, uint32_t z, uint32_t w)
{
	const uint32_t PRIME32_2 = 2246822519U, PRIME32_3 = 3266489917U;
	const uint32_t PRIME32_4 = 668265263U, PRIME32_5 = 374761393U;
	uint32_t h32 =  w + PRIME32_5 + x*PRIME32_3;
	h32 = PRIME32_4*((h32 << 17) | (h32 >> (32 - 17)));
	h32 += y * PRIME32_3;
	h32 = PRIME32_4*((h32 << 17) | (h32 >> (32 - 17)));
	h32 += z * PRIME32_3;
	h32 = PRIME32_4*((h32 << 17) | (h32 >> (32 - 17)));
	h32 = PRIME32_2*(h32^(h32 >> 15));
	h32 = PRIME32_3*(h32^(h32 >> 13));
	return h32^(h32 >> 16);
}

static inline uint64_t xxhash64(uint64_t x, uint64_t y, uint64_t z, uint64_t w)
{
	const uint64_t PRIME64_1 = 11400714785074694791ULL;
	const uint64_t PRIME64_2 = 14029467366897019727ULL;
	const uint64_t PRIME64_3 =  1609587929392839161ULL;
	const uint64_t PRIME64_4 =  9650029242287828579ULL;
	const uint64_t PRIME64_5 =  2870177450012600261ULL;

	#define ROL64(v, b) (((v) << (b)) | ((v) >> (64 - (b))))
	#define ROUND64(acc, inp) (ROL64((acc) + (inp) * PRIME64_2, 31) * PRIME64_1)

	uint64_t h64 = ROL64(ROUND64(0, x),  1)
				 + ROL64(ROUND64(0, y),  7)
				 + ROL64(ROUND64(0, z), 12)
				 + ROL64(ROUND64(0, w), 18);

	h64 = (h64 ^ ROUND64(0, x)) * PRIME64_1 + PRIME64_4;
	h64 = (h64 ^ ROUND64(0, y)) * PRIME64_1 + PRIME64_4;
	h64 = (h64 ^ ROUND64(0, z)) * PRIME64_1 + PRIME64_4;
	h64 = (h64 ^ ROUND64(0, w)) * PRIME64_1 + PRIME64_4;

	h64 ^= h64 >> 33; h64 *= PRIME64_2;
	h64 ^= h64 >> 29; h64 *= PRIME64_3;
	h64 ^= h64 >> 32;

	#undef ROUND64
	#undef ROL64

	return h64;
}

enum primes{
	P400 = 2749, P410 = 2833, P420 = 2909, P430 = 3001, P440 = 3083,
	P450 = 3187, P460 = 3259, P470 = 3343, P480 = 3433, P490 = 3517,
	P500 = 3581, P510 = 3659, P520 = 3733, P530 = 3823, P540 = 3911,
	P550 = 4001, P560 = 4073, P570 = 4153, P580 = 4241, P590 = 4327,
	P600 = 4421, P610 = 4507, P620 = 4591, P630 = 4663, P640 = 4759,
	P650 = 4861, P660 = 4943, P670 = 5009, P680 = 5099, P690 = 5189,
	P700 = 5281, P710 = 5393, P720 = 5449, P730 = 5527, P740 = 5641,
	P750 = 5701, P760 = 5801, P770 = 5861, P780 = 5953, P790 = 6067,
	P800 = 6143, P810 = 6229, P820 = 6311, P830 = 6373, P840 = 6481,
	P850 = 6577, P860 = 6679, P870 = 6763,
	P20K = 224743, P40K = 479939, P80K = 1020389, P160K = 2160563,
	P320K = 4562707, P640K = 9602471, P50KK = 982451653, P100KK = 2038074743
};


static inline void update_id(uint64_t* target, uint64_t a, uint64_t b, uint64_t c, uint64_t d)
{
	*target = xxhash64(a, b, c, d);
}

static inline bool point_in(rect r, float x, float y)
{
	return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

static inline float clampf(float v, float lo, float hi)
{
	return v < lo ? lo : v > hi ? hi : v;
}

static inline int utf8_next(const char* t, int i)
{
	if(!t[i]) return i;
	i++;
	while((t[i] & 0xC0) == 0x80) i++;
	return i;
}

static inline int utf8_prev(const char* t, int i)
{
	if(i <= 0) return 0;
	i--;
	while(i > 0 && (t[i] & 0xC0) == 0x80) i--;
	return i;
}

static inline void* hash_insert(r_hashmap* map, int64_t key);
static inline void  hash_clear(r_hashmap* map);

static inline int64_t hash_norm(int64_t key)
{
	return key == INT64_MIN ? INT64_MAX : key < 0 ? -key : key;
}

static inline int hash_slot(r_hashmap* map, int64_t key)
{
	uint64_t h = (uint64_t)key * 0x9E3779B97F4A7C15ull;
	return (int)((h ^ (h >> 32)) & (uint64_t)(map->num_slots - 1));
}

static inline void  hash_init  (r_hashmap* map, int num_slots, int sizeof_value)
{
	map->num_slots = num_slots;
	map->sizeof_value = sizeof_value;
	map->keys = malloc(num_slots * sizeof(int64_t));
	map->values = malloc(num_slots * map->sizeof_value);
	hash_clear(map);
}

static inline void* hash_value_at(r_hashmap* map, int index)
{
	return map->values + index * map->sizeof_value;
}

static inline void  hash_resize(r_hashmap* map, int new_num_slots)
{
	int      old_num_slots = map->num_slots;
	void*    old_values = map->values;
	int64_t* old_keys = map->keys;
	map->num_slots = new_num_slots;
	map->keys      = malloc(sizeof(int64_t)*new_num_slots);
	map->values    = malloc(new_num_slots * map->sizeof_value);
	hash_clear(map);

	if(old_values != NULL && old_keys != NULL)
	for(int i = 0; i < old_num_slots; i++)
	if(old_keys[i] >= 0)
		memcpy(hash_insert(map, old_keys[i]), (uint8_t*)old_values + i * map->sizeof_value, map->sizeof_value);

	free(old_values);
	free(old_keys);
}

static inline void  hash_clear(r_hashmap* map)
{
	for(int i = 0; i < map->num_slots; i++) map->keys[i] = -1;
	memset(map->values, 0, map->num_slots * map->sizeof_value);
}

static inline int hash_insert_slot(r_hashmap* map, int64_t key)
{
	int start = hash_slot(map, key), reuse = -1;
	for(int i = 0; i < 8; i++)
	{
		int idx = (start + i) & (map->num_slots - 1);
		if(map->keys[idx] == key) return idx;
		if(map->keys[idx] == -2 && reuse == -1) reuse = idx;
		if(map->keys[idx] == -1) return reuse != -1 ? reuse : idx;
	}
	return reuse;
}

static inline int hash_find(r_hashmap* map, int64_t key)
{
	key = hash_norm(key);
	int start = hash_slot(map, key);
	for(int i = 0; i < 8; i++)
	{
		int idx = (start + i) & (map->num_slots - 1);
		if(map->keys[idx] == -1)  return -1;
		if(map->keys[idx] == key) return idx;
	}
	return -1;
}

static inline void* hash_insert(r_hashmap* map, int64_t key)
{
	key = hash_norm(key);
	int index = hash_insert_slot(map, key);
	if(index == -1)
	{
		hash_resize(map, map->num_slots*2);
		return hash_insert(map, key);
	}
	map->keys[index] = key;
	return hash_value_at(map, index);
}

static inline void** hash_insertv(r_hashmap* map, int64_t key)
{
	return (void**)hash_insert(map, key);
}

static inline void* hash_get(r_hashmap* map, int64_t key)
{
	int index = hash_find(map, key);
	if(index == -1) return NULL;
	return hash_value_at(map, index);
}

static inline int hash_geti(r_hashmap* map, int64_t key, int default_val)
{
	int index = hash_find(map, key);
	if(index == -1) return default_val;
	return *(int*)hash_value_at(map, index);
}

static inline void* hash_getv(r_hashmap* map, int64_t key)
{
	int index = hash_find(map, key);
	if(index == -1) return NULL;
	return *(void**)hash_value_at(map, index);
}

static inline void hash_remove(r_hashmap* map, int64_t key)
{
	int index = hash_find(map, key);
	if(index != -1)
		map->keys[index] = -2;
}

static inline bool is_descendant(r_ui_list* l, r_ui_list* ancestor)
{
	for(r_ui_list* p = l->ephemeral_parent; p; p = p->ephemeral_parent)
	if(p == ancestor)
		return true;

	return false;
}

int break_text(r_context* c, int style, const char* text, int len, int width, bool force, int* w);

#endif
