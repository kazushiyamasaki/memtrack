/*
 * memtrack.c -- implementation part of a library to assist with memory-related
 *               debugging
 * version 0.9.0, June 12, 2025
 *
 * License: zlib License
 *
 * Copyright (c) 2025 Kazushi Yamasaki
 *
 * This software is provided ‘as-is’, without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 */

#include "memtrack.h"

#include <stdio.h>


#if !defined (__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
	#error "This program requires C99 or higher."
#endif


#ifndef MEMTRACK_DISABLE


#include <string.h>


#undef malloc
#undef calloc
#undef realloc
#undef free


#define MEMTRACK_ENTRIES_COUNT 64
#define MEMTRACK_ENTRIES_TRIAL 4


typedef struct {
	void* ptr;
	size_t size;
#ifdef DEBUG
	const char* alloc_file;
	const char* last_realloc_file;
	const char* free_file;
	int alloc_line;
	int last_realloc_line;
	int free_line;
	bool is_freed;
#endif
} MemTrackEntry;  /* パディングの削減のため順序がわかりにくくなっているので注意 */


static HashTable* memtrack_entries = NULL;


#define GLOBAL_LOCK_FUNC_NAME memtrack_lock
#define GLOBAL_UNLOCK_FUNC_NAME memtrack_unlock
#define GLOBAL_LOCK_FUNC_SCOPE 

#include "global_lock.h"


static void quit (void);

/* 重要: この関数は必ずロックした後に呼び出す必要があります！ */
static void init (void) {
	for (size_t i = 0; i < MEMTRACK_ENTRIES_TRIAL; i++) {
		memtrack_entries = ht_create(MEMTRACK_ENTRIES_COUNT);
		if (LIKELY(memtrack_entries != NULL)) break;
	}
	if (UNLIKELY(memtrack_entries == NULL)) {
		fprintf(stderr, "Failed to initialize memory tracking.\nFile: %s   Line: %d\n", __FILE__, __LINE__);
		memtrack_unlock();
		exit(EXIT_FAILURE);
	}

	atexit(quit);
}


void memtrack_entry_add (void* ptr, size_t size, const char* file, int line) {
	if (ptr == NULL) {
		fprintf(stderr, "ptr is null! Memory cannot be tracked!\nFile: %s   Line: %d\n", file, line);
		return;
	}

	if (UNLIKELY(memtrack_entries == NULL)) init();

	MemTrackEntry entry = {
		.ptr = ptr,
		.size = size
#ifdef DEBUG
		, 
		.alloc_file = file,
		.alloc_line = line,
		.last_realloc_file = NULL,
		.last_realloc_line = 0,
		.is_freed = false,
		.free_file = NULL,
		.free_line = 0
#endif
	};

	if (UNLIKELY(!ht_set(memtrack_entries, (key_type)ptr, &entry, sizeof(MemTrackEntry))))
		fprintf(stderr, "Failed to add entry to memory tracking.\nFile: %s   Line: %d\n", file, line);
}


void memtrack_entry_update (void* old_ptr, void* new_ptr, size_t new_size, const char* file, int line) {
	if (old_ptr == NULL) {
		memtrack_entry_add(new_ptr, new_size, file, line);
		return;
	}

	if (new_ptr == NULL) new_ptr = old_ptr;

	if (UNLIKELY(memtrack_entries == NULL)) {
		init();
		fprintf(stderr, "No entry found to update! The memory might not be tracked.\nFile: %s   Line: %d\n", file, line);
		memtrack_entry_add(new_ptr, new_size, file, line);
		return;
	}

	MemTrackEntry* old_entry = ht_get(memtrack_entries, (key_type)old_ptr);
	if (UNLIKELY(old_entry == NULL)) {
		fprintf(stderr, "No entry found to update! The memory might not be tracked.\nFile: %s   Line: %d\n", file, line);
		memtrack_entry_add(new_ptr, new_size, file, line);
		return;
	}

	if (old_ptr == new_ptr) {  /* 同じエントリを使える場合は処理を分けることで無駄な処理を減らす */
		old_entry->ptr = new_ptr;
		old_entry->size = new_size;
#ifdef DEBUG
		old_entry->last_realloc_file = file;
		old_entry->last_realloc_line = line;
#endif
		return;
	}

	MemTrackEntry new_entry = {
		.ptr = new_ptr,
		.size = new_size
#ifdef DEBUG
		, 
		.last_realloc_file = file,
		.last_realloc_line = line,  /* 更新はここまで、以下は旧エントリのコピー */
		.alloc_file = old_entry->alloc_file,
		.alloc_line = old_entry->alloc_line,
		.is_freed = old_entry->is_freed,
		.free_file = old_entry->free_file,
		.free_line = old_entry->free_line
#endif
	};

	if (UNLIKELY(!ht_set(memtrack_entries, (key_type)new_ptr, &new_entry, sizeof(MemTrackEntry))))
		fprintf(stderr, "Failed to add new entry to memory tracking.\nFile: %s   Line: %d\n", file, line);
}


void memtrack_entry_free (void* ptr, const char* file, int line) {
	if (ptr == NULL) return;

	if (UNLIKELY(memtrack_entries == NULL)) {
		init();
		fprintf(stderr, "No entry found to free! The memory might not be tracked.\nFile: %s   Line: %d\n", file, line);
		return;
	}

	MemTrackEntry* entry = ht_get(memtrack_entries, (key_type)ptr);
	if (entry == NULL) {
		fprintf(stderr, "No entry found to free! The memory might not be tracked.\nFile: %s   Line: %d\n", file, line);
		return;
	}

#ifndef DEBUG
	ht_delete(memtrack_entries, (key_type)ptr);
#else
	entry->is_freed = true;
	entry->free_file = file;
	entry->free_line = line;
#endif
}


void* memtrack_malloc_without_lock (size_t size, const char* file, int line) {
	if (size == 0) {
		fprintf(stderr, "No processing was done because the size is zero.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}

	void* ptr = malloc(size);
	if (UNLIKELY(ptr == NULL))
		fprintf(stderr, "Memory allocation failed.\nFile: %s   Line: %d\n", file, line);
	else
		memtrack_entry_add(ptr, size, file, line);
	return ptr;
}


void* memtrack_malloc (size_t size, const char* file, int line) {
	memtrack_lock();
	void* ptr = memtrack_malloc_without_lock(size, file, line);
	memtrack_unlock();
	return ptr;
}


void* memtrack_calloc_without_lock (size_t count, size_t size, const char* file, int line) {
	if (count == 0) {
		fprintf(stderr, "No processing was done because the count is zero.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	} else if (size == 0) {
		fprintf(stderr, "No processing was done because the size is zero.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	} else if (count > (SIZE_MAX / size)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}

	void* ptr = calloc(count, size);
	if (UNLIKELY(ptr == NULL))
		fprintf(stderr, "Memory allocation failed.\nFile: %s   Line: %d\n", file, line);
	else
		memtrack_entry_add(ptr, size * count, file, line);
	return ptr;
}


void* memtrack_calloc (size_t count, size_t size, const char* file, int line) {
	memtrack_lock();
	void* ptr = memtrack_calloc_without_lock(count, size, file, line);
	memtrack_unlock();
	return ptr;
}


void* memtrack_realloc_without_lock (void* ptr, size_t size, const char* file, int line) {
	if (size == 0) {
		fprintf(stderr, "Undefined behavior, do not use anymore. The memory block will be freed and NULL will be returned.\nFile: %s   Line: %d\n", file, line);
		memtrack_free_without_lock(ptr, file, line);
		return NULL;
	}

	void* new_ptr = realloc(ptr, size);
	if (UNLIKELY(new_ptr == NULL)) {
		fprintf(stderr, "Memory allocation failed.\nFile: %s   Line: %d\n", file, line);

	} else {
#if defined (__GNUC__) && !defined (__clang__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wuse-after-free"  /* memtrack自体のデバッグを行う際は必ず外すこと */
#endif

		memtrack_entry_update(ptr, new_ptr, size, file, line);

#if defined (__GNUC__) && !defined (__clang__)
	#pragma GCC diagnostic pop
#endif
	}
	return new_ptr;
}


void* memtrack_realloc (void* ptr, size_t size, const char* file, int line) {
	memtrack_lock();
	void* new_ptr = memtrack_realloc_without_lock(ptr, size, file, line);
	memtrack_unlock();
	return new_ptr;
}


void memtrack_free_without_lock (void* ptr, const char* file, int line) {
	if (ptr == NULL) return;

#ifdef DEBUG
	if (UNLIKELY(memtrack_entries == NULL)) {
		init();
		fprintf(stderr, "No entry found to free! The memory might not be tracked.\nFile: %s   Line: %d\n", file, line);
		free(ptr);
		return;
	}

	MemTrackEntry* entry = ht_get(memtrack_entries, (key_type)ptr);
	if (entry == NULL) {
		fprintf(stderr, "No entry found to free! The memory might not be tracked.\nFile: %s   Line: %d\n", file, line);
		free(ptr);
		return;
	}

	if (entry->is_freed) {
		fprintf(stderr, "Memory already freed!\nrefree File: %s   Line: %d\nfree File: %s   Line: %d\n", file, line, entry->free_file, entry->free_line);
		return;
	}
#endif

	free(ptr);

#if defined (__GNUC__) && !defined (__clang__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wuse-after-free"  /* memtrack自体のデバッグを行う際は必ず外すこと */
#endif

	memtrack_entry_free(ptr, file, line);

#if defined (__GNUC__) && !defined (__clang__)
	#pragma GCC diagnostic pop
#endif
}


void memtrack_free (void* ptr, const char* file, int line) {
	memtrack_lock();
	memtrack_free_without_lock(ptr, file, line);
	memtrack_unlock();
}


void* memtrack_recalloc_without_lock (void* ptr, size_t count, size_t size, const char* file, int line) {
	if (ptr == NULL)
		return memtrack_calloc_without_lock(count, size, file, line);

	size_t old_size = 0;

	if (UNLIKELY(memtrack_entries == NULL)) {
		init();
		fprintf(stderr, "No entry found to recalloc! The memory might not be tracked.\nFile: %s   Line: %d\n", file, line);

	} else {
		old_size = memtrack_get_size_without_lock(ptr, __FILE__, __LINE__);
		if (old_size == 0) {
			memtrack_entry_free(ptr, file, line);
			if (count != 0 && size != 0)
				return memtrack_calloc_without_lock(count, size, file, line);
			else return NULL;
		}
	}

	void* new_ptr = memtrack_realloc_array_without_lock(ptr, count, size, file, line);
	if (new_ptr == NULL) return new_ptr;

	size_t new_size = count * size;
	if (old_size < new_size)
		memset((char*)new_ptr + old_size, 0, new_size - old_size);

	return new_ptr;
}


void* memtrack_recalloc (void* ptr, size_t count, size_t size, const char* file, int line) {
	memtrack_lock();
	void* new_ptr = memtrack_recalloc_without_lock(ptr, count, size, file, line);
	memtrack_unlock();
	return new_ptr;
}


void* memtrack_malloc_array_without_lock (size_t count, size_t size, const char* file, int line) {
	if (size == 0) {
		fprintf(stderr, "No processing was done because the size is zero.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	} else if (count > (SIZE_MAX / size)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}
	return memtrack_malloc_without_lock(count * size, file, line);
}


void* memtrack_malloc_array (size_t count, size_t size, const char* file, int line) {
	memtrack_lock();
	void* ptr = memtrack_malloc_array_without_lock(count, size, file, line);
	memtrack_unlock();
	return ptr;
}


void* memtrack_calloc_array (size_t count, size_t size, const char* file, int line) {
	return memtrack_calloc(count, size, file, line);
}


void* memtrack_realloc_array_without_lock (void* ptr, size_t count, size_t size, const char* file, int line) {
	if (size == 0) {
		fprintf(stderr, "Undefined behavior, do not use anymore. The memory block will be freed and NULL will be returned.\nFile: %s   Line: %d\n", file, line);
		memtrack_free_without_lock(ptr, file, line);
		return NULL;
	} else if (count > (SIZE_MAX / size)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}
	return memtrack_realloc_without_lock(ptr, count * size, file, line);
}


void* memtrack_realloc_array (void* ptr, size_t count, size_t size, const char* file, int line) {
	memtrack_lock();
	void* new_ptr = memtrack_realloc_array_without_lock(ptr, count, size, file, line);
	memtrack_unlock();
	return new_ptr;
}


void* memtrack_recalloc_array (void* ptr, size_t count, size_t size, const char* file, int line) {
	return memtrack_recalloc(ptr, count, size, file, line);
}


size_t memtrack_get_size_without_lock (void* ptr, const char* file, int line) {
	if (ptr == NULL) {
		fprintf(stderr, "Cannot return value because ptr is NULL.\nFile: %s   Line: %d\n", file, line);
		return 0;
	}

	if (UNLIKELY(memtrack_entries == NULL)) {
		init();
		fprintf(stderr, "No entry found to get size! The memory might not be tracked.\nFile: %s   Line: %d\n", file, line);
		return 0;
	}

	MemTrackEntry* entry = ht_get(memtrack_entries, (key_type)ptr);
	if (entry == NULL) {
		fprintf(stderr, "No entry found to get size! The memory might not be tracked.\nFile: %s   Line: %d\n", file, line);
		return 0;
	}

	return entry->size;
}


size_t memtrack_get_size (void* ptr, const char* file, int line) {
	memtrack_lock();
	size_t size = memtrack_get_size_without_lock(ptr, file, line);
	memtrack_unlock();
	return size;
}


void memtrack_all_check (void) {
	size_t memtrack_entries_arr_cnt;
	MemTrackEntry** memtrack_entries_arr;
	for (size_t i = 0; i < MEMTRACK_ENTRIES_TRIAL; i++) {
		memtrack_entries_arr = (MemTrackEntry**)ht_all_get(memtrack_entries, &memtrack_entries_arr_cnt);
		if (LIKELY(memtrack_entries_arr != NULL)) break;
	}
	if (UNLIKELY(memtrack_entries_arr == NULL)) {
		fprintf(stderr, "Failed to get all entries from memory tracking.\nFile: %s   Line: %d\n", __FILE__, __LINE__);

	} else {
		printf("\n");
		for (size_t i = 0; i < memtrack_entries_arr_cnt; i++) {
			if (UNLIKELY(memtrack_entries_arr[i] == NULL)) {
				fprintf(stderr, "Entry is NULL!\nFile: %s   Line: %d\n", __FILE__, __LINE__);
			} else if (UNLIKELY(memtrack_entries_arr[i]->ptr == NULL)) {
				fprintf(stderr, "Entry pointer is NULL!\nFile: %s   Line: %d\n", __FILE__, __LINE__);
			} else {
#ifndef DEBUG
				printf("\nAlready Freed: false\nPointer: %p   Size: %zu\nPlease use debug mode if you need more detailed information.\n", memtrack_entries_arr[i]->ptr, memtrack_entries_arr[i]->size);
#else
				if (memtrack_entries_arr[i]->is_freed) {
					if (memtrack_entries_arr[i]->last_realloc_file != NULL)
						printf("\nAlready Freed: true\nPointer: %p   Size: %zu\nfree File: %s   Line: %d\nalloc File: %s   Line: %d\nLast realloc File: %s   Line: %d\n", memtrack_entries_arr[i]->ptr, memtrack_entries_arr[i]->size, memtrack_entries_arr[i]->free_file, memtrack_entries_arr[i]->free_line, memtrack_entries_arr[i]->alloc_file, memtrack_entries_arr[i]->alloc_line, memtrack_entries_arr[i]->last_realloc_file, memtrack_entries_arr[i]->last_realloc_line);
					else
						printf("\nAlready Freed: true\nPointer: %p   Size: %zu\nfree File: %s   Line: %d\nalloc File: %s   Line: %d\n", memtrack_entries_arr[i]->ptr, memtrack_entries_arr[i]->size, memtrack_entries_arr[i]->free_file, memtrack_entries_arr[i]->free_line, memtrack_entries_arr[i]->alloc_file, memtrack_entries_arr[i]->alloc_line);
				} else {
					if (memtrack_entries_arr[i]->last_realloc_file != NULL)
						printf("\nAlready Freed: false\nPointer: %p   Size: %zu\nalloc File: %s   Line: %d\nLast realloc File: %s   Line: %d\n", memtrack_entries_arr[i]->ptr, memtrack_entries_arr[i]->size, memtrack_entries_arr[i]->alloc_file, memtrack_entries_arr[i]->alloc_line, memtrack_entries_arr[i]->last_realloc_file, memtrack_entries_arr[i]->last_realloc_line);
					else
						printf("\nAlready Freed: false\nPointer: %p   Size: %zu\nalloc File: %s   Line: %d\n", memtrack_entries_arr[i]->ptr, memtrack_entries_arr[i]->size, memtrack_entries_arr[i]->alloc_file, memtrack_entries_arr[i]->alloc_line);
				}
#endif
			}
		}
		ht_all_release_arr(memtrack_entries_arr);
		printf("\n\n");
	}
}


static void quit (void) {
	size_t memtrack_entries_arr_cnt;
	MemTrackEntry** memtrack_entries_arr;
	for (size_t i = 0; i < MEMTRACK_ENTRIES_TRIAL; i++) {
		memtrack_entries_arr = (MemTrackEntry**)ht_all_get(memtrack_entries, &memtrack_entries_arr_cnt);
		if (LIKELY(memtrack_entries_arr != NULL)) break;
	}
	if (UNLIKELY(memtrack_entries_arr == NULL)) {
		fprintf(stderr, "Failed to get all entries from memory tracking.\nFile: %s   Line: %d\n", __FILE__, __LINE__);

	} else {
		for (size_t i = 0; i < memtrack_entries_arr_cnt; i++) {
			if (UNLIKELY(memtrack_entries_arr[i] == NULL)) {
				fprintf(stderr, "Entry is NULL!\nFile: %s   Line: %d\n", __FILE__, __LINE__);
			} else if (UNLIKELY(memtrack_entries_arr[i]->ptr == NULL)) {
				fprintf(stderr, "Entry pointer is NULL!\nFile: %s   Line: %d\n", __FILE__, __LINE__);
			} else {
#ifndef DEBUG
				memtrack_free_without_lock(memtrack_entries_arr[i]->ptr, __FILE__, __LINE__);
#else
				if (UNLIKELY(!memtrack_entries_arr[i]->is_freed)) {
					fprintf(stderr, "\nMemory not freed!\nPointer: %p   Size: %zu\nalloc File: %s   Line: %d\nLast realloc File: %s   Line: %d\n", memtrack_entries_arr[i]->ptr, memtrack_entries_arr[i]->size, memtrack_entries_arr[i]->alloc_file, memtrack_entries_arr[i]->alloc_line, memtrack_entries_arr[i]->last_realloc_file, memtrack_entries_arr[i]->last_realloc_line);
					memtrack_free_without_lock(memtrack_entries_arr[i]->ptr, __FILE__, __LINE__);
				}
#endif
			}
		}
		ht_all_release_arr(memtrack_entries_arr);
	}
	ht_destroy(memtrack_entries);
	memtrack_entries = NULL;

	global_lock_quit();
}


#else  /* defined MEMTRACK_DISABLE */


void* malloc_array (size_t count, size_t size) {
	if (size == 0) return NULL;
	if (count > (SIZE_MAX / size)) return NULL;
	return malloc(count * size);
}


void* calloc_array (size_t count, size_t size) {
	return calloc(count, size);
}


void* realloc_array (void* ptr, size_t count, size_t size) {
	if (size == 0) {
		free(ptr);
		return NULL;
	}
	if (count > (SIZE_MAX / size)) return NULL;
	return realloc(ptr, count * size);
}


#endif
