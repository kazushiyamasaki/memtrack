/*
 * memtrack_alloc_nd_array.h -- interface of a library that adds alloc_nd_array to
 *                              the management target of memtrack.h
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
 *
 *
 *
 * IMPORTANT:
 * This library relies on the functionality of memtrack.h. Please use it with caution
 * after reading the IMPORTANT section in memtrack.h.
 */

#pragma once

#ifndef MEMTRACK_ALLOC_ND_ARRAY_H
#define MEMTRACK_ALLOC_ND_ARRAY_H


#ifndef MEMTRACK_DISABLE


#include "alloc_nd_array.h"
#include "memtrack.h"


MHT_CPP_C_BEGIN


/*
 * Replaces alloc_nd_array and calloc_nd_array with memory-tracking versions.
 * To disable this behavior, define MEMTRACK_DISABLE_REPLACE_ANDA_FUNC
 * macro before including this file.
 */
#ifndef MEMTRACK_DISABLE_REPLACE_ANDA_FUNC
	#define alloc_nd_array(sizes, dims, elem_size) memtrack_alloc_nd_array((sizes), (dims), (elem_size), __FILE__, __LINE__)
	#define calloc_nd_array(sizes, dims, elem_size) memtrack_calloc_nd_array((sizes), (dims), (elem_size), __FILE__, __LINE__)
	#define free_nd_array(array) memtrack_free_nd_array((array), __FILE__, __LINE__)
#endif


/*
 * memtrack_alloc_nd_array
 * @param sizes: array containing sizes for each dimension (must have length equal to dims)
 * @param dims: number of array dimensions (designed for 2+ dimensions but supports 1D arrays)
 * @param elem_size: size of each element in bytes (e.g., sizeof(int), sizeof(double), etc.)
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to the multi-dimensional array or NULL on failure
 * @note: After calling, cast the returned pointer to the appropriate type (e.g., int***, double**, etc.) to access it as the multi-dimensional array. The allocated memory must be freed using free() when no longer needed. The returned memory is uninitialized (use calloc_nd_array if zero-initialization is desired).
 */
extern void* memtrack_alloc_nd_array (const size_t* sizes, size_t dims, size_t elem_size, const char* file, int line);

/*
 * memtrack_calloc_nd_array
 * @param sizes: array containing sizes for each dimension (must have length equal to dims)
 * @param dims: number of array dimensions (designed for 2+ dimensions but supports 1D arrays)
 * @param elem_size: size of each element in bytes (e.g., sizeof(int), sizeof(double), etc.)
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to the multi-dimensional array or NULL on failure
 * @note: After calling, cast the returned pointer to the appropriate type (e.g., int***, double**, etc.) to access it as the multi-dimensional array. The allocated memory must be freed using free() when no longer needed. Use alloc_nd_array if zero-initialization is not required and higher performance is desired.
 */
extern void* memtrack_calloc_nd_array (const size_t* sizes, size_t dims, size_t elem_size, const char* file, int line);

/*
 * memtrack_free_nd_array
 * @param array: pointer to the multi-dimensional array allocated by alloc_nd_array or calloc_nd_array
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @note: this function frees the multi-dimensional array (the actual memory block allocated for it)
 */
extern void memtrack_free_nd_array (void* array, const char* file, int line);


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
 * memtrack_alloc_nd_array_without_lock
 * @param sizes: array containing sizes for each dimension (must have length equal to dims)
 * @param dims: number of array dimensions (designed for 2+ dimensions but supports 1D arrays)
 * @param elem_size: size of each element in bytes (e.g., sizeof(int), sizeof(double), etc.)
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to the multidimensional array or NULL on failure
 * @note: After calling, cast the returned pointer to the appropriate type (e.g., int***, double**, etc.) to access it as a multidimensional array. The allocated memory must be freed using free() when no longer needed. The returned memory is uninitialized (use calloc_nd_array if zero-initialization is desired).
 */
extern void* memtrack_alloc_nd_array_without_lock (const size_t* sizes, size_t dims, size_t elem_size, const char* file, int line);

/*
 * memtrack_calloc_nd_array_without_lock
 * @param sizes: array containing sizes for each dimension (must have length equal to dims)
 * @param dims: number of array dimensions (designed for 2+ dimensions but supports 1D arrays)
 * @param elem_size: size of each element in bytes (e.g., sizeof(int), sizeof(double), etc.)
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to the multidimensional array or NULL on failure
 * @note: After calling, cast the returned pointer to the appropriate type (e.g., int***, double**, etc.) to access it as a multidimensional array. The allocated memory must be freed using free() when no longer needed. Use alloc_nd_array if zero-initialization is not required and higher performance is desired.
 */
extern void* memtrack_calloc_nd_array_without_lock (const size_t* sizes, size_t dims, size_t elem_size, const char* file, int line);


MHT_CPP_C_END


#endif


#endif
