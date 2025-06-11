/*
 * memtrack_alloc_nd_array.c -- implementation part of a library that adds
 *                              alloc_nd_array to the management target of memtrack.h
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

#if !defined (__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
	#error "This program requires C99 or higher."
#endif


#ifndef MEMTRACK_DISABLE


#include "memtrack_alloc_nd_array.h"

#include <stdio.h>


#undef alloc_nd_array
#undef calloc_nd_array


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


static size_t align_up (size_t value, size_t alignment) {
	if (alignment == 0 || alignment == 1) return value;

	if (value > (SIZE_MAX - (alignment - 1))) return 0;  /* オーバーフロー対策 */

	if ((alignment & (alignment - 1)) == 0) {  /* alignment が2のべき乗ならビット演算 */
		return (value + (alignment - 1)) & ~(alignment - 1);
	}
	return ((value + (alignment - 1)) / alignment) * alignment;  /* それ以外なら割り算 */
}


size_t mem_anda_size_check (const size_t* sizes, size_t dims, size_t elem_size, const char* file, int line) {
	if (elem_size == 0) {
		fprintf(stderr, "No processing was done because elem_size is zero.\nFile: %s   Line: %d\n", file, line);
		return 0;
	}
	if(dims == 0) {
		fprintf(stderr, "No processing was done because dims is zero.\nFile: %s   Line: %d\n", file, line);
		return 0;
	}

	if (dims == 1) { 
		if (sizes[0] == 0) {
			fprintf(stderr, "No processing was done because sizes[0] is zero.\nFile: %s   Line: %d\n", file, line);
			return 0;
		}
		if (sizes[0] > (SIZE_MAX / elem_size)) {
			fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
			return 0;
		}
		return sizes[0] * elem_size;
	}

	/* 総要素数 (データ部分) と各階層のポインタ数の合計を計算 */
	size_t total_elements = 1;
	size_t total_ptrs = 0;
	for (size_t i = 0; i < dims; i++) {
		if (sizes[i] == 0) {
			fprintf(stderr, "No processing was done because sizes[%zu] is zero.\nFile: %s   Line: %d\n", i, file, line);
			return 0;
		}
		if (total_elements > (SIZE_MAX / sizes[i])) {
			fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
			return 0;
		}
		total_elements *= sizes[i];

		if (i < dims - 1)  /* 最下層以外はポインタ数を加算 */
			total_ptrs += total_elements;
	}

	if (total_ptrs > (SIZE_MAX / sizeof(void*))) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
		return 0;
	}
	size_t size_ptrs = total_ptrs * sizeof(void*);

	/* アラインメント違反を防ぐため必要に応じて切り上げた値を取得 */
	if (elem_size > sizeof(void*)) {
		size_t size_ptrs_max = align_up(size_ptrs, elem_size);
		if (size_ptrs_max == 0) {
			return 0;
		}
		size_ptrs = size_ptrs_max;
	}

	if (total_elements > (SIZE_MAX / elem_size)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
		return 0;
	}
	size_t size_data = total_elements * elem_size;

	if (size_ptrs > (SIZE_MAX - size_data)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
		return 0;
	}
	return size_ptrs + size_data;
}


void* memtrack_alloc_nd_array_without_lock (const size_t* sizes, size_t dims, size_t elem_size, const char* file, int line) {
	size_t a_size = mem_anda_size_check(sizes, dims, elem_size, file, line);
	if (a_size == 0) return NULL;

	void* ptr = alloc_nd_array(sizes, dims, elem_size);
	if (UNLIKELY(ptr == NULL)) {
		fprintf(stderr, "Memory allocation failed.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}

	memtrack_entry_add(ptr, a_size, file, line);
	return ptr;
}


void* memtrack_alloc_nd_array (const size_t* sizes, size_t dims, size_t elem_size, const char* file, int line) {
	memtrack_lock();
	void* ptr = memtrack_alloc_nd_array_without_lock(sizes, dims, elem_size, file, line);
	memtrack_unlock();
	return ptr;
}


void* memtrack_calloc_nd_array_without_lock (const size_t* sizes, size_t dims, size_t elem_size, const char* file, int line) {
	size_t a_size = mem_anda_size_check(sizes, dims, elem_size, file, line);
	if (a_size == 0) return NULL;

	void* ptr = calloc_nd_array(sizes, dims, elem_size);
	if (UNLIKELY(ptr == NULL)) {
		fprintf(stderr, "Memory allocation failed.\nFile: %s   Line: %d\n", file, line);
		return NULL;
	}

	memtrack_entry_add(ptr, a_size, file, line);
	return ptr;
}


void* memtrack_calloc_nd_array (const size_t* sizes, size_t dims, size_t elem_size, const char* file, int line) {
	memtrack_lock();
	void* ptr = memtrack_calloc_nd_array_without_lock(sizes, dims, elem_size, file, line);
	memtrack_unlock();
	return ptr;
}


void memtrack_free_nd_array (void* array, const char* file, int line) {
	memtrack_free(array, file, line);
}


#endif
