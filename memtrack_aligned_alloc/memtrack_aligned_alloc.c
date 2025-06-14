/*
 * memtrack_aligned_alloc.c -- implementation part of a library that adds
 *                             aligned_alloc to the management target of memtrack.h
 * version 0.9.2, June 15, 2025
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
#include <errno.h>


#if !defined (__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)
	#error "This program requires C11 or higher."
#endif


#if defined (__unix__) || defined (__linux__) || defined (__APPLE__)
	#include <unistd.h>
#endif

#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)) || defined(_POSIX_VERSION) || defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
	#define MAYBE_ERRNO_THREAD_LOCAL
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
		memtrack_errfunc = "memtrack_aligned_alloc";
		return NULL;
	}

	if (alignment < sizeof(void*)) {
		fprintf(stderr, "Alignment must be greater than or equal to sizeof(void*).\nFile: %s   Line: %d\n", file, line);
		errno = EINVAL;
		memtrack_errfunc = "memtrack_aligned_alloc";
		return NULL;
	}

	if (size == 0) {
		fprintf(stderr, "No processing was done because size is zero.\nFile: %s   Line: %d\n", file, line);
		errno = EINVAL;
		memtrack_errfunc = "memtrack_aligned_alloc";
		return NULL;
	}

	if (size < alignment) {
		fprintf(stderr, "Size must be greater than or equal to alignment.\nFile: %s   Line: %d\n", file, line);
		errno = EINVAL;
		memtrack_errfunc = "memtrack_aligned_alloc";
		return NULL;
	}

	if ((size % alignment) != 0) {
		fprintf(stderr, "Size must be a multiple of alignment.\nFile: %s   Line: %d\n", file, line);
		errno = EINVAL;
		memtrack_errfunc = "memtrack_aligned_alloc";
		return NULL;
	}

	void* ptr = aligned_alloc(alignment, size);
	if (UNLIKELY(ptr == NULL)) {
		fprintf(stderr, "Memory allocation failed.\nFile: %s   Line: %d\n", file, line);
		errno = ENOMEM;
		memtrack_errfunc = "memtrack_aligned_alloc";
	}
	return ptr;
}


void* memtrack_aligned_alloc_without_lock (size_t alignment, size_t size, const char* file, int line) {
	void* ptr = memtrack_aligned_alloc_without_entry_add(alignment, size, file, line);

	if (ptr != NULL) {
#ifdef MAYBE_ERRNO_THREAD_LOCAL
		int tmp_errno = errno;
#endif
		errno = 0;

		memtrack_entry_add(ptr, size, file, line);

		if (UNLIKELY(errno != 0)) memtrack_errfunc = "memtrack_aligned_alloc";
#ifdef MAYBE_ERRNO_THREAD_LOCAL
		else errno = tmp_errno;
#endif
	}

	return ptr;
}


void* memtrack_aligned_alloc (size_t alignment, size_t size, const char* file, int line) {
	memtrack_lock();
	void* ptr = memtrack_aligned_alloc_without_lock(alignment, size, file, line);
	memtrack_unlock();
	return ptr;
}


void* memtrack_aligned_calloc_without_lock (size_t alignment, size_t count, size_t size, const char* file, int line) {
	if (count == 0) {
		fprintf(stderr, "No processing was done because the count is zero.\nFile: %s   Line: %d\n", file, line);
		errno = EINVAL;
		memtrack_errfunc = "memtrack_aligned_calloc";
		return NULL;
	} else if (size > (SIZE_MAX / count)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
		errno = EINVAL;
		memtrack_errfunc = "memtrack_aligned_calloc";
		return NULL;
	}

	void* ptr = memtrack_aligned_alloc_without_lock(alignment, count * size, file, line);
	if (ptr == NULL) {
		memtrack_errfunc = "memtrack_aligned_calloc";
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
	if (ptr == NULL) {
		void* new_ptr = memtrack_aligned_alloc_without_lock(alignment, size, file, line);
		if (new_ptr == NULL)
			memtrack_errfunc = "memtrack_aligned_realloc";
		return new_ptr;
	}

	if (size == 0) {
		fprintf(stderr, "Undefined behavior because the size is zero, do not use anymore. The memory block will be freed and NULL will be returned.\nFile: %s   Line: %d\n", file, line);
		errno = EINVAL;
		memtrack_errfunc = "memtrack_aligned_realloc";

		memtrack_free_without_lock(ptr, file, line);
		return NULL;
	}

	size_t old_size = memtrack_get_size_without_lock(ptr, __FILE__, __LINE__);
	if (old_size == 0) {
		memtrack_errfunc = "memtrack_aligned_realloc";
		return NULL;
	}

	void* new_ptr = memtrack_aligned_alloc_without_entry_add(alignment, size, file, line);
	if (new_ptr == NULL) {
		memtrack_errfunc = "memtrack_aligned_realloc";
		return NULL;
	}

	size_t copy_size;
	if (old_size < size)
		copy_size = old_size;
	else
		copy_size = size;

	memcpy(new_ptr, ptr, copy_size);

#ifdef MAYBE_ERRNO_THREAD_LOCAL
	int tmp_errno = errno;
#endif
	errno = 0;

	memtrack_entry_update(ptr, new_ptr, size, file, line);

	if (UNLIKELY(errno != 0)) memtrack_errfunc = "memtrack_aligned_realloc";
#ifdef MAYBE_ERRNO_THREAD_LOCAL
	else errno = tmp_errno;
#endif

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
	if (ptr == NULL) {
		void* new_ptr = memtrack_aligned_calloc_without_lock(alignment, count, size, file, line);
		if (new_ptr == NULL)
			memtrack_errfunc = "memtrack_aligned_recalloc";
		return new_ptr;
	}

	size_t old_size = memtrack_get_size_without_lock(ptr, __FILE__, __LINE__);
	if (old_size == 0) {
		memtrack_errfunc = "memtrack_aligned_recalloc";
		return NULL;
	}

	void* new_ptr = memtrack_aligned_realloc_array_without_lock(ptr, alignment, count, size, file, line);
	if (new_ptr == NULL) {
		memtrack_errfunc = "memtrack_aligned_recalloc";
		return NULL;
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
	if (count == 0) {
		fprintf(stderr, "No processing was done because the count is zero.\nFile: %s   Line: %d\n", file, line);
		errno = EINVAL;
		memtrack_errfunc = "memtrack_aligned_alloc_array";
		return NULL;
	} else if (size > (SIZE_MAX / count)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
		errno = EINVAL;
		memtrack_errfunc = "memtrack_aligned_alloc_array";
		return NULL;
	}

	void* ptr = memtrack_aligned_alloc_without_lock(alignment, count * size, file, line);
	if (ptr == NULL)
		memtrack_errfunc = "memtrack_aligned_alloc_array";
	return ptr;
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
	if (count == 0) {
		fprintf(stderr, "Undefined behavior because the count is zero, do not use anymore. The memory block will be freed and NULL will be returned.\nFile: %s   Line: %d\n", file, line);
		errno = EINVAL;
		memtrack_errfunc = "memtrack_aligned_realloc_array";

		memtrack_free_without_lock(ptr, file, line);
		return NULL;
	} else if (size > (SIZE_MAX / count)) {
		fprintf(stderr, "Memory allocation overflow.\nFile: %s   Line: %d\n", file, line);
		errno = EINVAL;
		memtrack_errfunc = "memtrack_aligned_realloc_array";
		return NULL;
	}

	void* new_ptr = memtrack_aligned_realloc_without_lock(ptr, alignment, count * size, file, line);
	if (new_ptr == NULL)
		memtrack_errfunc = "memtrack_aligned_realloc_array";
	return new_ptr;
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
	if (!ht_is_power_of_two(alignment)) {
		memtrack_errfunc = "aligned_calloc";
		return NULL;
	}

	if (alignment < sizeof(void*) || count == 0 || size == 0 ||
		size > (SIZE_MAX / count) ||
		(count * size) < alignment ||
		((count * size) % alignment) != 0) {
		errno = EINVAL;
		memtrack_errfunc = "aligned_calloc";
		return NULL;
	}

	void* ptr = aligned_alloc(alignment, count * size);
	if (ptr == NULL) {
		errno = ENOMEM;
		memtrack_errfunc = "aligned_calloc";
		return NULL;
	}
	memset(ptr, 0, count * size);
	return ptr;
}


void* aligned_alloc_array (size_t alignment, size_t count, size_t size) {
	if (!ht_is_power_of_two(alignment)) {
		memtrack_errfunc = "aligned_alloc_array";
		return NULL;
	}

	if (alignment < sizeof(void*) || count == 0 || size == 0 ||
		size > (SIZE_MAX / count) ||
		(count * size) < alignment ||
		((count * size) % alignment) != 0) {
		errno = EINVAL;
		memtrack_errfunc = "aligned_alloc_array";
		return NULL;
	}

	void* ptr = aligned_alloc(alignment, count * size);
	if (ptr == NULL) {
		errno = ENOMEM;
		memtrack_errfunc = "aligned_alloc_array";
		return NULL;
	}
	return ptr;
}


void* aligned_calloc_array (size_t alignment, size_t count, size_t size){
	return aligned_calloc(alignment, count, size);
}


#endif
