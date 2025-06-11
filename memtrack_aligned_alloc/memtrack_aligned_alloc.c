/*
 * memtrack_aligned_alloc.c -- implementation part of a library that adds
 *                             aligned_alloc to the management target of memtrack.h
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

#include "memtrack_aligned_alloc.h"

#include <stdio.h>
#include <string.h>


#if !defined (__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)
	#error "This program requires C11 or higher."
#endif


#ifndef MEMTRACK_DISABLE


#undef free
#undef aligned_alloc


#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"

	#define LIKELY(x)   __builtin_expect(!!(x), 1)
	#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#pragma GCC diagnostic pop
#else
	#define LIKELY(x)   (x)
	#define UNLIKELY(x) (x)
#endif


static void* memtrack_aligned_alloc_without_entry_add (size_t alignment, size_t size, const char* file, int line) {
	if (!ht_is_power_of_two(alignment)) {
		fprintf(stderr, "Alignment must be a power of 2.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}

	if (alignment < sizeof(void*)) {
		fprintf(stderr, "Alignment must be greater than or equal to sizeof(void*).\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}

	if (size == 0) {
		fprintf(stderr, "No processing was done because size is zero.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}

	if (size < alignment) {
		fprintf(stderr, "Size must be greater than or equal to alignment.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}

	if ((size % alignment) != 0) {
		fprintf(stderr, "Size must be a multiple of alignment.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}

	void* ptr = aligned_alloc(alignment, size);
	if (UNLIKELY(ptr == NULL))
		fprintf(stderr, "Memory allocation failed.\nFile: %s   Line: %d\n", file, line);
	return ptr;
}


void* memtrack_aligned_alloc_without_lock (size_t alignment, size_t size, const char* file, int line) {
	void* ptr = memtrack_aligned_alloc_without_entry_add(alignment, size, file, line);
	if (ptr != NULL)
		memtrack_entry_add(ptr, size, file, line);
	return ptr;
}


void* memtrack_aligned_alloc (size_t alignment, size_t size, const char* file, int line) {
	memtrack_lock();
	void* ptr = memtrack_aligned_alloc_without_lock(alignment, size, file, line);
	memtrack_unlock();
	return ptr;
}


void* memtrack_aligned_calloc_without_lock (size_t alignment, size_t count, size_t size, const char* file, int line) {
	if (size == 0) {
		fprintf(stderr, "No processing was done because the size is zero.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	} else if (count > (SIZE_MAX / size)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}

	void* ptr = memtrack_aligned_alloc_without_lock(alignment, count * size, file, line);
	if (ptr == NULL) {
		return NULL;
	}
	memset(ptr, 0, count * size);
	return ptr;
}


void* memtrack_aligned_calloc (size_t alignment, size_t count, size_t size, const char* file, int line) {
	memtrack_lock();
	void* ptr = memtrack_aligned_calloc_without_lock(alignment, count, size, file, line);
	memtrack_unlock();
	return ptr;
}


void* memtrack_aligned_realloc_without_lock (void* ptr, size_t alignment, size_t size, const char* file, int line) {
	if (ptr == NULL)
		return memtrack_aligned_alloc_without_lock(alignment, size, file, line);

	if (size == 0) {
		memtrack_free_without_lock(ptr, file, line);
		return NULL;
	}

	size_t old_size = memtrack_get_size_without_lock(ptr, __FILE__, __LINE__);
	if (old_size == 0) return NULL;

	void* new_ptr = memtrack_aligned_alloc_without_entry_add(alignment, size, file, line);
	if (new_ptr == NULL) {
		return new_ptr;
	}

	size_t copy_size;
	if (old_size < size)
		copy_size = old_size;
	else
		copy_size = size;
	memcpy(new_ptr, ptr, copy_size);

	memtrack_entry_update(ptr, new_ptr, size, file, line);

	free(ptr); /* By design, ptr will always be different from new_ptr. */
	return new_ptr;
}


void* memtrack_aligned_realloc (void* ptr, size_t alignment, size_t size, const char* file, int line) {
	memtrack_lock();
	void* new_ptr = memtrack_aligned_realloc_without_lock(ptr, alignment, size, file, line);
	memtrack_unlock();
	return new_ptr;
}


void* memtrack_aligned_recalloc_without_lock (void* ptr, size_t alignment, size_t count, size_t size, const char* file, int line) {
	if (ptr == NULL)
		return memtrack_aligned_calloc_without_lock(alignment, count, size, file, line);

	size_t old_size = memtrack_get_size_without_lock(ptr, __FILE__, __LINE__);
	if (old_size == 0) return NULL;

	void* new_ptr = memtrack_aligned_realloc_array_without_lock(ptr, alignment, count, size, file, line);
	if (new_ptr == NULL) {
		return new_ptr;
	}

	size_t new_size = count * size;
	if (old_size < new_size)
		memset((char*)new_ptr + old_size, 0, new_size - old_size);
	return new_ptr;
}


void* memtrack_aligned_recalloc (void* ptr, size_t alignment, size_t count, size_t size, const char* file, int line) {
	memtrack_lock();
	void* new_ptr = memtrack_aligned_recalloc_without_lock(ptr, alignment, count, size, file, line);
	memtrack_unlock();
	return new_ptr;
}


void* memtrack_aligned_alloc_array_without_lock (size_t alignment, size_t count, size_t size, const char* file, int line) {
	if (size == 0) {
		fprintf(stderr, "No processing was done because the size is zero.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	} else if (count > (SIZE_MAX / size)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}
	return memtrack_aligned_alloc_without_lock(alignment, count * size, file, line);
}


void* memtrack_aligned_alloc_array (size_t alignment, size_t count, size_t size, const char* file, int line) {
	memtrack_lock();
	void* ptr = memtrack_aligned_alloc_array_without_lock(alignment, count, size, file, line);
	memtrack_unlock();
	return ptr;
}


void* memtrack_aligned_calloc_array (size_t alignment, size_t count, size_t size, const char* file, int line) {
	return memtrack_aligned_calloc(alignment, count, size, file, line);
}


void* memtrack_aligned_realloc_array_without_lock (void* ptr, size_t alignment, size_t count, size_t size, const char* file, int line) {
	if (size == 0) {
		memtrack_free_without_lock(ptr, file, line);
		return NULL;
	} else if (count > (SIZE_MAX / size)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}
	return memtrack_aligned_realloc_without_lock(ptr, alignment, count * size, file, line);
}


void* memtrack_aligned_realloc_array (void* ptr, size_t alignment, size_t count, size_t size, const char* file, int line) {
	memtrack_lock();
	void* new_ptr = memtrack_aligned_realloc_array_without_lock(ptr, alignment, count, size, file, line);
	memtrack_unlock();
	return new_ptr;
}


void* memtrack_aligned_recalloc_array (void* ptr, size_t alignment, size_t count, size_t size, const char* file, int line) {
	return memtrack_aligned_recalloc(ptr, alignment, count, size, file, line);
}


#else  /* defined MEMTRACK_DISABLE */


void* aligned_calloc (size_t alignment, size_t count, size_t size) {
	if (size == 0) return NULL;
	if (count > (SIZE_MAX / size)) return NULL;

	void* ptr = aligned_alloc(alignment, count * size);
	if (ptr == NULL) {
		return NULL;
	}
	memset(ptr, 0, count * size);
	return ptr;
}


void* aligned_alloc_array (size_t alignment, size_t count, size_t size) {
	if (size == 0) return NULL;
	if (count > (SIZE_MAX / size)) return NULL;

	return aligned_alloc(alignment, count * size);
}


void* aligned_calloc_array (size_t alignment, size_t count, size_t size){
	return aligned_calloc(alignment, count, size);
}


#endif
