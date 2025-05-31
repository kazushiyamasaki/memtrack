/*
 * memtrack.c -- implementation part of a library to assist with memory-related
 *               debugging
 * version 0.9.2, May 31, 2025
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
#include <string.h>


#undef malloc
#undef calloc
#undef realloc
#undef free


#if !defined (__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
	#error "This program requires C99 or higher."
#endif


#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0600)
	#error "This program requires Windows Vista or later. Define _WIN32_WINNT accordingly."
#endif


#define MEMTRACK_ENTRIES_COUNT 64
#define MEMTRACK_ENTRIES_TRIAL 4


typedef struct {
	void* ptr;
	size_t size;
#ifdef DEBUG
	const char* alloc_file;
	const char* last_realloc_file;
	const char* free_file;
	unsigned int alloc_line;
	int last_realloc_line;
	unsigned int free_line;
	bool is_freed;
#endif
} MemTrackEntry;  /* パディングの削減のため順序がわかりにくくなっているので注意 */


static HashTable* memtrack_entries = NULL;



#if defined (__unix__) || defined (__linux__) || defined (__APPLE__)
	#include <unistd.h>
#endif


#if defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined (__STDC_NO_THREADS__)
	#define C11_THREADS_AVAILABLE

	#include <threads.h>
	static mtx_t memtrack_lock_mutex;
	static once_flag mtx_init_once = ONCE_FLAG_INIT;

	static void init_mtx (void) {
		if (mtx_init(&memtrack_lock_mutex, mtx_plain) != thrd_success) {
			fprintf(stderr, "Failed to initialize the mutex!\nFile: %s   Line: %u\n", __FILE__, __LINE__);
			exit(EXIT_FAILURE);
		}
	}
#elif defined (_POSIX_THREADS) && (_POSIX_THREADS > 0)
	#define PTHREAD_AVAILABLE

	#include <pthread.h>
	static pthread_mutex_t memtrack_lock_mutex = PTHREAD_MUTEX_INITIALIZER;
#elif defined (_WIN32)
	#include <windows.h>
	static INIT_ONCE cs_init_once = INIT_ONCE_STATIC_INIT;
	static CRITICAL_SECTION memtrack_lock_cs;

	static BOOL CALLBACK InitCriticalSection (PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context) {
		(void)InitOnce;  (void)Parameter;  (void)Context;
		InitializeCriticalSection(&memtrack_lock_cs);
		return true;
	}

	static bool winver_checked = false;
	static bool is_windows_vista_or_later (void) {
		OSVERSIONINFO osvi = {0};
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);  /* 必要な初期化 */
		if (!GetVersionEx(&osvi)) {
			return false;
		}
		return (osvi.dwMajorVersion >= 6);
	}
#elif defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined (__STDC_NO_ATOMICS__)
	#define STDSTOMIC_AVAILABLE

	#include <stdatomic.h>
	static atomic_flag memtrack_lock_flag = ATOMIC_FLAG_INIT;
#elif defined (__GNUC__)
	#if defined (__has_builtin)
		#if __has_builtin (__sync_lock_test_and_set)
			#define GCC_SYNC_BUILTIN_AVAILABLE

			static volatile int memtrack_lock_int = 0;
		#else
			#error "No valid locking mechanism found on this platform."
		#endif
	#else
		#define GCC_SYNC_BUILTIN_AVAILABLE

		static volatile int memtrack_lock_int = 0;
	#endif
#else
	#error "No valid locking mechanism found on this platform."
#endif


#if !defined (C11_THREADS_AVAILABLE) && !defined (PTHREAD_AVAILABLE) && !defined (_WIN32)
	#if (defined (__x86_64__) || defined (__amd64__) || defined (_M_X64) || defined (__i386__) || defined (_M_IX86))
		#include <emmintrin.h>
		#define SPIN_WAIT() _mm_pause()
	#elif defined (_POSIX_PRIORITY_SCHEDULING)
		#include <sched.h>
		#define SPIN_WAIT() sched_yield()
	#else
		#define SPIN_WAIT() do { volatile size_t i; for (i = 0; i < 1000; ++i) { __asm__ __volatile__ ("" ::: "memory"); } } while (0)
	#endif
#endif


void memtrack_lock (void) {
#ifdef C11_THREADS_AVAILABLE
	call_once(&mtx_init_once, init_mtx);

	mtx_lock(&memtrack_lock_mutex);
#elif defined (PTHREAD_AVAILABLE)
	pthread_mutex_lock(&memtrack_lock_mutex);
#elif defined (_WIN32)
	if (winver_checked == false) {
		if (!is_windows_vista_or_later())
			exit(EXIT_FAILURE);
		else
			winver_checked = true;
	}  /* なるべく実行回数を減らしたいだけなので、複数回実行されても問題はない */

	InitOnceExecuteOnce(&cs_init_once, InitCriticalSection, NULL, NULL);

	EnterCriticalSection(&memtrack_lock_cs);
#elif defined (STDSTOMIC_AVAILABLE)
	while (atomic_flag_test_and_set_explicit(&memtrack_lock_flag, memory_order_acquire)) {
		SPIN_WAIT();
	}
#elif defined (GCC_SYNC_BUILTIN_AVAILABLE)
	while (__sync_lock_test_and_set(&memtrack_lock_int, 1)) {
		SPIN_WAIT();
	}
#endif
}


void memtrack_unlock (void) {
#ifdef C11_THREADS_AVAILABLE
	mtx_unlock(&memtrack_lock_mutex);
#elif defined (PTHREAD_AVAILABLE)
	pthread_mutex_unlock(&memtrack_lock_mutex);
#elif defined (_WIN32)
	LeaveCriticalSection(&memtrack_lock_cs);
#elif defined (STDSTOMIC_AVAILABLE)
	atomic_flag_clear_explicit(&memtrack_lock_flag, memory_order_release);
#elif defined (GCC_SYNC_BUILTIN_AVAILABLE)
    __sync_lock_release(&memtrack_lock_int);
#endif
}



static void quit (void);

/* 重要: この関数は必ずロックした後に呼び出す必要があります！ */
static void init (void) {
	for (size_t i = 0; i < MEMTRACK_ENTRIES_TRIAL; i++) {
		memtrack_entries = ht_create(MEMTRACK_ENTRIES_COUNT);
		if (memtrack_entries != NULL) break;
	}
	if (memtrack_entries == NULL) {
		fprintf(stderr, "Failed to initialize memory tracking.\nFile: %s   Line: %u\n", __FILE__, __LINE__);
		memtrack_unlock();
		exit(EXIT_FAILURE);
	}

	atexit(quit);
}


void memtrack_entry_add (void* ptr, size_t size, const char* file, unsigned int line) {
	if (ptr == NULL) {
		fprintf(stderr, "ptr is null! Memory cannot be tracked!\nFile: %s   Line: %u\n", file, line);
		return;
	}

	if (memtrack_entries == NULL) init();

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

	if (!ht_set(memtrack_entries, (key_type)ptr, &entry, sizeof(MemTrackEntry)))
		fprintf(stderr, "Failed to add entry to memory tracking.\nFile: %s   Line: %u\n", file, line);
}


void memtrack_entry_update (void* old_ptr, void* new_ptr, size_t new_size, const char* file, unsigned int line) {
	if (old_ptr == NULL) {
		memtrack_entry_add(new_ptr, new_size, file, line);
		return;
	}

	if (new_ptr == NULL) new_ptr = old_ptr;

	if (memtrack_entries == NULL) {
		init();
		fprintf(stderr, "No entry found to update! The memory might not be tracked.\nFile: %s   Line: %u\n", file, line);
		memtrack_entry_add(new_ptr, new_size, file, line);
		return;
	}

	MemTrackEntry* entry = ht_get(memtrack_entries, (key_type)old_ptr);
	if (entry == NULL) {
		fprintf(stderr, "No entry found to update! The memory might not be tracked.\nFile: %s   Line: %u\n", file, line);
		memtrack_entry_add(new_ptr, new_size, file, line);
		return;
	}

	entry->ptr = new_ptr;
	entry->size = new_size;
#ifdef DEBUG
	entry->last_realloc_file = file;
	entry->last_realloc_line = line;
#endif
}


void memtrack_entry_free (void* ptr, const char* file, unsigned int line) {
	if (ptr == NULL) return;

	if (memtrack_entries == NULL) {
		init();
		fprintf(stderr, "No entry found to free! The memory might not be tracked.\nFile: %s   Line: %u\n", file, line);
		return;
	}

	MemTrackEntry* entry = ht_get(memtrack_entries, (key_type)ptr);
	if (entry == NULL) {
		fprintf(stderr, "No entry found to free! The memory might not be tracked.\nFile: %s   Line: %u\n", file, line);
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


void* memtrack_malloc_without_lock (size_t size, const char* file, unsigned int line) {
	if (size == 0) {
		fprintf(stderr, "No processing was done because the size is zero.\nFile: %s   Line: %u\n", file, line);
		return NULL;
	}

	void* ptr = malloc(size);
	if (ptr == NULL)
		fprintf(stderr, "Memory allocation failed.\nFile: %s   Line: %u\n", file, line);
	else
		memtrack_entry_add(ptr, size, file, line);
	return ptr;
}


void* memtrack_malloc (size_t size, const char* file, unsigned int line) {
	memtrack_lock();
	void* ptr = memtrack_malloc_without_lock(size, file, line);
	memtrack_unlock();
	return ptr;
}


void* memtrack_calloc_without_lock (size_t count, size_t size, const char* file, unsigned int line) {
	if (count == 0) {
		fprintf(stderr, "No processing was done because the count is zero.\nFile: %s   Line: %u\n", file, line);
		return NULL;
	} else if (size == 0) {
		fprintf(stderr, "No processing was done because the size is zero.\nFile: %s   Line: %u\n", file, line);
		return NULL;
	} else if (count > (SIZE_MAX / size)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %u\n", file, line);
		return NULL;
	}

	void* ptr = calloc(count, size);
	if (ptr == NULL)
		fprintf(stderr, "Memory allocation failed.\nFile: %s   Line: %u\n", file, line);
	else
		memtrack_entry_add(ptr, size * count, file, line);
	return ptr;
}


void* memtrack_calloc (size_t count, size_t size, const char* file, unsigned int line) {
	memtrack_lock();
	void* ptr = memtrack_calloc_without_lock(count, size, file, line);
	memtrack_unlock();
	return ptr;
}


void memtrack_free_without_lock (void* ptr, const char* file, unsigned int line);

void* memtrack_realloc_without_lock (void* ptr, size_t size, const char* file, unsigned int line) {
	if (size == 0) {
		fprintf(stderr, "Undefined behavior, do not use anymore. The memory block will be freed and NULL will be returned.\nFile: %s   Line: %u\n", file, line);
		memtrack_free_without_lock(ptr, file, line);
		return NULL;
	}

	void* new_ptr = realloc(ptr, size);
	if (new_ptr == NULL)
		fprintf(stderr, "Memory allocation failed.\nFile: %s   Line: %u\n", file, line);
	else
		memtrack_entry_update(ptr, new_ptr, size, file, line);
	return new_ptr;
}


void* memtrack_realloc (void* ptr, size_t size, const char* file, unsigned int line) {
	memtrack_lock();
	void* new_ptr = memtrack_realloc_without_lock(ptr, size, file, line);
	memtrack_unlock();
	return new_ptr;
}


void memtrack_free_without_lock (void* ptr, const char* file, unsigned int line) {
	if (ptr == NULL) return;

#ifdef DEBUG
	if (memtrack_entries == NULL) {
		init();
		fprintf(stderr, "No entry found to free! The memory might not be tracked.\nFile: %s   Line: %u\n", file, line);
		free(ptr);
		return;
	}

	MemTrackEntry* entry = ht_get(memtrack_entries, (key_type)ptr);
	if (entry == NULL) {
		fprintf(stderr, "No entry found to free! The memory might not be tracked.\nFile: %s   Line: %u\n", file, line);
		free(ptr);
		return;
	}

	if (entry->is_freed) {
		fprintf(stderr, "Memory already freed!\nrefree File: %s   Line: %u\nfree File: %s   Line: %u\n", file, line, entry->free_file, entry->free_line);
		return;
	}
#endif

	free(ptr);
	memtrack_entry_free(ptr, file, line);
}


void memtrack_free (void* ptr, const char* file, unsigned int line) {
	memtrack_lock();
	memtrack_free_without_lock(ptr, file, line);
	memtrack_unlock();
}


void* memtrack_realloc_array_without_lock (void* ptr, size_t count, size_t size, const char* file, unsigned int line);
size_t memtrack_get_size_without_lock (void* ptr, const char* file, unsigned int line);

void* memtrack_recalloc_without_lock (void* ptr, size_t count, size_t size, const char* file, unsigned int line) {
	if (ptr == NULL)
		return memtrack_calloc_without_lock(count, size, file, line);

	size_t old_size = 0;

	if (memtrack_entries == NULL) {
		init();
		fprintf(stderr, "No entry found to recalloc! The memory might not be tracked.\nFile: %s   Line: %u\n", file, line);

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


void* memtrack_recalloc (void* ptr, size_t count, size_t size, const char* file, unsigned int line) {
	memtrack_lock();
	void* new_ptr = memtrack_recalloc_without_lock(ptr, count, size, file, line);
	memtrack_unlock();
	return new_ptr;
}


void* memtrack_malloc_array_without_lock (size_t count, size_t size, const char* file, unsigned int line) {
	if (size == 0) {
		fprintf(stderr, "No processing was done because the size is zero.\nFile: %s   Line: %u\n", file, line);
		return NULL;
	} else if (count > (SIZE_MAX / size)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %u\n", file, line);
		return NULL;
	}
	return memtrack_malloc_without_lock(count * size, file, line);
}


void* memtrack_malloc_array (size_t count, size_t size, const char* file, unsigned int line) {
	memtrack_lock();
	void* ptr = memtrack_malloc_array_without_lock(count, size, file, line);
	memtrack_unlock();
	return ptr;
}


void* memtrack_calloc_array (size_t count, size_t size, const char* file, unsigned int line) {
	return memtrack_calloc(count, size, file, line);
}


void* memtrack_realloc_array_without_lock (void* ptr, size_t count, size_t size, const char* file, unsigned int line) {
	if (size == 0) {
		fprintf(stderr, "Undefined behavior, do not use anymore. The memory block will be freed and NULL will be returned.\nFile: %s   Line: %u\n", file, line);
		memtrack_free_without_lock(ptr, file, line);
		return NULL;
	} else if (count > (SIZE_MAX / size)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %u\n", file, line);
		return NULL;
	}
	return memtrack_realloc_without_lock(ptr, count * size, file, line);
}


void* memtrack_realloc_array (void* ptr, size_t count, size_t size, const char* file, unsigned int line) {
	memtrack_lock();
	void* new_ptr = memtrack_realloc_array_without_lock(ptr, count, size, file, line);
	memtrack_unlock();
	return new_ptr;
}


void* memtrack_recalloc_array (void* ptr, size_t count, size_t size, const char* file, unsigned int line) {
	return memtrack_recalloc(ptr, count, size, file, line);
}


size_t memtrack_get_size_without_lock (void* ptr, const char* file, unsigned int line) {
	if (ptr == NULL) {
		fprintf(stderr, "Cannot return value because ptr is NULL.\nFile: %s   Line: %u\n", file, line);
		return 0;
	}

	if (memtrack_entries == NULL) {
		init();
		fprintf(stderr, "No entry found to get size! The memory might not be tracked.\nFile: %s   Line: %u\n", file, line);
		return 0;
	}

	MemTrackEntry* entry = ht_get(memtrack_entries, (key_type)ptr);
	if (entry == NULL) {
		fprintf(stderr, "No entry found to get size! The memory might not be tracked.\nFile: %s   Line: %u\n", file, line);
		return 0;
	}

	return entry->size;
}


size_t memtrack_get_size (void* ptr, const char* file, unsigned int line) {
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
		if (memtrack_entries_arr != NULL) break;
	}
	if (memtrack_entries_arr == NULL) {
		fprintf(stderr, "Failed to get all entries from memory tracking.\nFile: %s   Line: %u\n", __FILE__, __LINE__);

	} else {
		printf("\n");
		for (size_t i = 0; i < memtrack_entries_arr_cnt; i++) {
			if (memtrack_entries_arr[i] == NULL) {
				fprintf(stderr, "Entry is NULL!\nFile: %s   Line: %u\n", __FILE__, __LINE__);
			} else if (memtrack_entries_arr[i]->ptr == NULL) {
				fprintf(stderr, "Entry pointer is NULL!\nFile: %s   Line: %u\n", __FILE__, __LINE__);
			} else {
#ifndef DEBUG
				printf("\nAlready Freed: false\nPointer: %p   Size: %zu\nPlease use debug mode if you need more detailed information.\n", memtrack_entries_arr[i]->ptr, memtrack_entries_arr[i]->size);
#else
				if (memtrack_entries_arr[i]->is_freed) {
					if (memtrack_entries_arr[i]->last_realloc_file != NULL)
						printf("\nAlready Freed: true\nPointer: %p   Size: %zu\nfree File: %s   Line: %u\nalloc File: %s   Line: %u\nLast realloc File: %s   Line: %u\n", memtrack_entries_arr[i]->ptr, memtrack_entries_arr[i]->size, memtrack_entries_arr[i]->free_file, memtrack_entries_arr[i]->free_line, memtrack_entries_arr[i]->alloc_file, memtrack_entries_arr[i]->alloc_line, memtrack_entries_arr[i]->last_realloc_file, memtrack_entries_arr[i]->last_realloc_line);
					else
						printf("\nAlready Freed: true\nPointer: %p   Size: %zu\nfree File: %s   Line: %u\nalloc File: %s   Line: %u\n", memtrack_entries_arr[i]->ptr, memtrack_entries_arr[i]->size, memtrack_entries_arr[i]->free_file, memtrack_entries_arr[i]->free_line, memtrack_entries_arr[i]->alloc_file, memtrack_entries_arr[i]->alloc_line);
				} else {
					if (memtrack_entries_arr[i]->last_realloc_file != NULL)
						printf("\nAlready Freed: false\nPointer: %p   Size: %zu\nalloc File: %s   Line: %u\nLast realloc File: %s   Line: %u\n", memtrack_entries_arr[i]->ptr, memtrack_entries_arr[i]->size, memtrack_entries_arr[i]->alloc_file, memtrack_entries_arr[i]->alloc_line, memtrack_entries_arr[i]->last_realloc_file, memtrack_entries_arr[i]->last_realloc_line);
					else
						printf("\nAlready Freed: false\nPointer: %p   Size: %zu\nalloc File: %s   Line: %u\n", memtrack_entries_arr[i]->ptr, memtrack_entries_arr[i]->size, memtrack_entries_arr[i]->alloc_file, memtrack_entries_arr[i]->alloc_line);
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
		if (memtrack_entries_arr != NULL) break;
	}
	if (memtrack_entries_arr == NULL) {
		fprintf(stderr, "Failed to get all entries from memory tracking.\nFile: %s   Line: %u\n", __FILE__, __LINE__);

	} else {
		for (size_t i = 0; i < memtrack_entries_arr_cnt; i++) {
			if (memtrack_entries_arr[i] == NULL) {
				fprintf(stderr, "Entry is NULL!\nFile: %s   Line: %u\n", __FILE__, __LINE__);
			} else if (memtrack_entries_arr[i]->ptr == NULL) {
				fprintf(stderr, "Entry pointer is NULL!\nFile: %s   Line: %u\n", __FILE__, __LINE__);
			} else {
#ifndef DEBUG
				memtrack_free_without_lock(memtrack_entries_arr[i]->ptr, __FILE__, __LINE__);
#else
				if (!memtrack_entries_arr[i]->is_freed) {
					fprintf(stderr, "\nMemory not freed!\nPointer: %p   Size: %zu\nalloc File: %s   Line: %u\nLast realloc File: %s   Line: %u\n", memtrack_entries_arr[i]->ptr, memtrack_entries_arr[i]->size, memtrack_entries_arr[i]->alloc_file, memtrack_entries_arr[i]->alloc_line, memtrack_entries_arr[i]->last_realloc_file, memtrack_entries_arr[i]->last_realloc_line);
					memtrack_free_without_lock(memtrack_entries_arr[i]->ptr, __FILE__, __LINE__);
				}
#endif
			}
		}
		ht_all_release_arr(memtrack_entries_arr);
	}
	ht_destroy(memtrack_entries);
	memtrack_entries = NULL;

#ifdef C11_THREADS_AVAILABLE
	mtx_destroy(&memtrack_lock_mutex);
#elif defined (PTHREAD_AVAILABLE)
	pthread_mutex_destroy(&memtrack_lock_mutex);
#elif defined (_WIN32)
	DeleteCriticalSection(&memtrack_lock_cs);
#endif
}
