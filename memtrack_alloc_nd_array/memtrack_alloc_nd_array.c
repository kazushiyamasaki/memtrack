/*
 * memtrack_alloc_nd_array.c -- implementation part of a library that adds
 *                              alloc_nd_array to the management target of memtrack.h
 * version 0.9.3, June 15, 2025
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

#if !defined (__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
	#error "This program requires C99 or higher."
#endif


#include "memtrack_alloc_nd_array.h"

#include <stdio.h>
#include <errno.h>


#undef malloc
#undef calloc
#undef alloc_nd_array
#undef calloc_nd_array


#if defined (__unix__) || defined (__linux__) || defined (__APPLE__)
	#include <unistd.h>
#endif

#if (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)) || defined (_POSIX_VERSION) || defined (__linux__) || defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || defined (__DragonFly__)
	#define MAYBE_ERRNO_THREAD_LOCAL
#endif


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


static void* calloc_wrapper (size_t size) {
	return calloc(1, size);
}


void* memtrack_alloc_nd_array_without_lock (const size_t sizes[], size_t dims, size_t elem_size, const char* file, int line) {
	size_t size_ptrs, size_padding, total_elements;
	if (!calculate_nd_array_size(sizes, dims, elem_size, &size_ptrs, &size_padding, &total_elements)) {
		fprintf(stderr, "Invalid parameters for nd-array allocation.\nFile: %s   Line: %d\n", file, line);
		memtrack_errfunc = "memtrack_alloc_nd_array";
		return NULL;
	}

	void* ptr = allocate_and_initialize_nd_array(sizes, dims, elem_size, size_ptrs, size_padding, total_elements, malloc);
	if (UNLIKELY(ptr == NULL)) {
		fprintf(stderr, "Memory allocation failed.\nFile: %s   Line: %d\n", file, line);
		memtrack_errfunc = "memtrack_alloc_nd_array";
		return NULL;
	}

#ifdef MAYBE_ERRNO_THREAD_LOCAL
	int tmp_errno = errno;
#endif
	errno = 0;

	memtrack_entry_add(ptr, size_ptrs + size_padding + (total_elements * elem_size), file, line);

	if (UNLIKELY(errno != 0)) memtrack_errfunc = "memtrack_alloc_nd_array";
#ifdef MAYBE_ERRNO_THREAD_LOCAL
	else errno = tmp_errno;
#endif

	return ptr;
}


void* memtrack_alloc_nd_array (const size_t sizes[], size_t dims, size_t elem_size, const char* file, int line) {
	memtrack_lock();
	void* ptr = memtrack_alloc_nd_array_without_lock(sizes, dims, elem_size, file, line);
	memtrack_unlock();
	return ptr;
}


void* memtrack_calloc_nd_array_without_lock (const size_t sizes[], size_t dims, size_t elem_size, const char* file, int line) {
	size_t size_ptrs, size_padding, total_elements;
	if (!calculate_nd_array_size(sizes, dims, elem_size, &size_ptrs, &size_padding, &total_elements)) {
		fprintf(stderr, "Invalid parameters for nd-array allocation.\nFile: %s   Line: %d\n", file, line);
		memtrack_errfunc = "memtrack_calloc_nd_array";
		return NULL;
	}

	void* ptr = allocate_and_initialize_nd_array(sizes, dims, elem_size, size_ptrs, size_padding, total_elements, calloc_wrapper);
	if (UNLIKELY(ptr == NULL)) {
		fprintf(stderr, "Memory allocation failed.\nFile: %s   Line: %d\n", file, line);
		memtrack_errfunc = "memtrack_calloc_nd_array";
		return NULL;
	}

#ifdef MAYBE_ERRNO_THREAD_LOCAL
	int tmp_errno = errno;
#endif
	errno = 0;

	memtrack_entry_add(ptr, size_ptrs + size_padding + (total_elements * elem_size), file, line);

	if (UNLIKELY(errno != 0)) memtrack_errfunc = "memtrack_calloc_nd_array";
#ifdef MAYBE_ERRNO_THREAD_LOCAL
	else errno = tmp_errno;
#endif

	return ptr;
}


void* memtrack_calloc_nd_array (const size_t sizes[], size_t dims, size_t elem_size, const char* file, int line) {
	memtrack_lock();
	void* ptr = memtrack_calloc_nd_array_without_lock(sizes, dims, elem_size, file, line);
	memtrack_unlock();
	return ptr;
}


void memtrack_free_nd_array (void* array, const char* file, int line) {
#ifdef MAYBE_ERRNO_THREAD_LOCAL
	int tmp_errno = errno;
#endif
	errno = 0;

	memtrack_free(array, file, line);

	if (UNLIKELY(errno != 0)) memtrack_errfunc = "memtrack_free_nd_array";
#ifdef MAYBE_ERRNO_THREAD_LOCAL
	else errno = tmp_errno;
#endif
}
