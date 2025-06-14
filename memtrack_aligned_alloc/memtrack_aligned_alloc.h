/*
 * memtrack_aligned_alloc.h -- interface of a library that adds aligned_alloc to the
 *                             management target of memtrack.h
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
 *
 *
 *
 * IMPORTANT:
 * This library relies on the functionality of memtrack.h. Please use it with caution
 * after reading the IMPORTANT section in memtrack.h.
 */

#pragma once

#ifndef MEMTRACK_ALIGNED_ALLOC_H
#define MEMTRACK_ALIGNED_ALLOC_H


#include "memtrack.h"


MHT_CPP_C_BEGIN


#ifndef MEMTRACK_DISABLE


/*
 * Replaces aligned_alloc with memory-tracking versions.
 * To disable this behavior, define MEMTRACK_AA_DISABLE_REPLACE_STANDARD_FUNC macro
 * before including this file.
 */
#ifndef MEMTRACK_AA_DISABLE_REPLACE_STANDARD_FUNC
	#define aligned_alloc(alignment, size) memtrack_aligned_alloc((alignment), (size), __FILE__, __LINE__)
#endif

/*
 * The functions replaced by the following macros are specific to this library.
 * It is recommended not to remove them unless a conflict occurs.
 */
#define aligned_calloc(alignment, count, size) memtrack_aligned_calloc((alignment), (count), (size), __FILE__, __LINE__)
#define aligned_realloc(ptr, alignment, size) memtrack_aligned_realloc((ptr), (alignment), (size), __FILE__, __LINE__)
#define aligned_recalloc(ptr, alignment, count, size) memtrack_aligned_recalloc((ptr), (alignment), (count), (size), __FILE__, __LINE__)
#define aligned_alloc_array(alignment, count, size) memtrack_aligned_alloc_array((alignment), (count), (size), __FILE__, __LINE__)
#define aligned_calloc_array(alignment, count, size) memtrack_aligned_calloc_array((alignment), (count), (size), __FILE__, __LINE__)
#define aligned_realloc_array(ptr, alignment, count, size) memtrack_aligned_realloc_array((ptr), (alignment), (count), (size), __FILE__, __LINE__)



/*
 * memtrack_aligned_alloc
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param size: size of the memory block to allocate, displays a message and returns NULL if size is 0 or not a multiple of alignment
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 * @note: in C11 TC3 and later, if size is not a multiple of alignment, aligned_alloc returns NULL.
 */
extern void* memtrack_aligned_alloc (size_t alignment, size_t size, const char* file, int line);


/*
 * memtrack_aligned_calloc
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 * @note: displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* memtrack_aligned_calloc (size_t alignment, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_aligned_realloc
 * @param ptr: pointer to the memory block to reallocate
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param size: new size of the memory block in bytes, displays a message and returns NULL if size is 0 or not a multiple of alignment
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 */
extern void* memtrack_aligned_realloc (void* ptr, size_t alignment, size_t size, const char* file, int line);

/*
 * memtrack_aligned_recalloc
 * @param ptr: pointer to the memory block to reallocate
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: new number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: new size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 * @note: newly allocated memory beyond the original size is zero-initialized, displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* memtrack_aligned_recalloc (void* ptr, size_t alignment, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_aligned_alloc_array
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 * @note: displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* memtrack_aligned_alloc_array (size_t alignment, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_aligned_calloc_array
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 * @note: this function is a wrapper for memtrack_aligned_calloc, displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* memtrack_aligned_calloc_array (size_t alignment, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_aligned_realloc_array
 * @param ptr: pointer to the memory block to reallocate
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: new number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: new size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 * @note: displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* memtrack_aligned_realloc_array (void* ptr, size_t alignment, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_aligned_recalloc_array
 * @param ptr: pointer to the memory block to reallocate
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: new number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: new size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 * @note: this function is a wrapper for memtrack_aligned_recalloc, displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* memtrack_aligned_recalloc_array (void* ptr, size_t alignment, size_t count, size_t size, const char* file, int line);



/*
 * You must not use functions declared before this comment within the lock/unlock block.
 *
 * If you want to manipulate memory tracking entries, you can use the functions below,
 * but it's not recommended unless you're creating a wrapper for a function that needs
 * to be freed with free().
 * 
 * The following functions must not be used outside of a lock. When working outside the
 * lock, only use functions whose names do not end with _without_lock.
 */


/*
 * memtrack_aligned_alloc_without_lock
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param size: size of the memory block to allocate, displays a message and returns NULL if size is 0 or not a multiple of alignment
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 */
extern void* memtrack_aligned_alloc_without_lock (size_t alignment, size_t size, const char* file, int line);

/*
 * memtrack_aligned_calloc_without_lock
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 * @note: displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* memtrack_aligned_calloc_without_lock (size_t alignment, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_aligned_realloc_without_lock
 * @param ptr: pointer to the memory block to reallocate
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param size: new size of the memory block in bytes, displays a message and returns NULL if size is 0 or not a multiple of alignment
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 */
extern void* memtrack_aligned_realloc_without_lock (void* ptr, size_t alignment, size_t size, const char* file, int line);

/*
 * memtrack_aligned_recalloc_without_lock
 * @param ptr: pointer to the memory block to reallocate
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: new number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: new size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 * @note: newly allocated memory beyond the original size is zero-initialized, displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* memtrack_aligned_recalloc_without_lock (void* ptr, size_t alignment, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_aligned_alloc_array_without_lock
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 * @note: displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* memtrack_aligned_alloc_array_without_lock (size_t alignment, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_aligned_realloc_array_without_lock
 * @param ptr: pointer to the memory block to reallocate
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: new number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: new size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 * @note: displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* memtrack_aligned_realloc_array_without_lock (void* ptr, size_t alignment, size_t count, size_t size, const char* file, int line);


#else  /* defined MEMTRACK_DISABLE */


/*
 * IMPORTANT:
 * The functions aligned_realloc, aligned_realloc_array, aligned_recalloc and
 * aligned_recalloc_array are not supported in the no-tracking build.
 */


/*
 * aligned_calloc
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @return: pointer to allocated memory block, or NULL on failure
 * @note: displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* aligned_calloc (size_t alignment, size_t count, size_t size);

/*
 * aligned_alloc_array
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @return: pointer to allocated memory block, or NULL on failure
 * @note: displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* aligned_alloc_array (size_t alignment, size_t count, size_t size);

/*
 * aligned_calloc_array
 * @param alignment: alignment of the memory block, displays a message and returns NULL if alignment is not a power of 2 and greater than or equal to sizeof(void*)
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @return: pointer to allocated memory block, or NULL on failure
 * @note: this function is a wrapper for aligned_calloc, displays a message and returns NULL if count * size is not a multiple of alignment
 */
extern void* aligned_calloc_array (size_t alignment, size_t count, size_t size);


#endif


MHT_CPP_C_END


#endif
