/*
 * memtrack.h -- interface of a library to assist with memory-related debugging
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
 * This header replaces malloc, calloc, realloc, and free with memory-tracking versions
 * using macros. To avoid unintended side effects in other headers, make sure to include
 * this header after all standard or third-party headers.
 *
 * BAD (may break other code):
 *     #include "memtrack.h"
 *     #include <some_lib.h>
 *
 * GOOD:
 *     #include <some_lib.h>
 *     #include "memtrack.h"
 *
 * The functions malloc, calloc, realloc, and free are replaced by macros and therefore
 * cannot be passed to function pointers. Similarly, recalloc, malloc_array,
 * calloc_array, realloc_array, recalloc_array, and get_size are also implemented as
 * macros and cannot be passed to function pointers. If you need to use these functions
 * with function pointers, please use the actual functions they expand to, which are
 * prefixed with memtrack_.
 * 
 * You can disable the replacement of malloc, calloc, realloc, and free functions by
 * defining the MEMTRACK_DISABLE_REPLACE_STANDARD_FUNC macro before including this file.
 *
 *
 * Note:
 * This library uses a global lock to ensure thread safety. As a result, performance
 * may degrade significantly when accessed concurrently by many threads.
 * In high-load environments or those with many threads, it is recommended to design
 * your application to minimize simultaneous access whenever possible.
 *
 * To enable debug mode, define DEBUG macro before including this file.
 *
 * This library depends on the mhashtable library.
 */

#pragma once

#ifndef MEMTRACK_H
#define MEMTRACK_H


#include "mhashtable.h"


MHT_CPP_C_BEGIN


#include <stddef.h>
#include <stdlib.h>


/*
 * memtrack_errfunc is a global variable that stores the name of the function
 * where the most recent error occurred within the hash table library.
 *
 * It is set to NULL when no error has occurred.
 * This variable is used to provide more informative error diagnostics,
 * especially in combination with errno.
 *
 * It is recommended to check this variable and errno after calling
 * any library function that may fail.
 */
extern const char* memtrack_errfunc;


#ifndef MEMTRACK_DISABLE


/*
 * Replaces malloc, calloc, realloc, and free with memtrack_ versions.
 */
#ifndef MEMTRACK_DISABLE_REPLACE_STANDARD_FUNC
	#define malloc(size) memtrack_malloc((size), __FILE__, __LINE__)
	#define calloc(count, size) memtrack_calloc((count), (size), __FILE__, __LINE__)
	#define realloc(ptr, size) memtrack_realloc((ptr), (size), __FILE__, __LINE__)
	#define free(ptr) memtrack_free((ptr), __FILE__, __LINE__)
#endif

/*
 * The functions replaced by the following macros are specific to this library.
 * It is recommended not to remove them unless a conflict occurs.
 */
#define recalloc(ptr, count, size) memtrack_recalloc((ptr), (count), (size), __FILE__, __LINE__)
#define malloc_array(count, size) memtrack_malloc_array((count), (size), __FILE__, __LINE__)
#define calloc_array(count, size) memtrack_calloc_array((count), (size), __FILE__, __LINE__)
#define realloc_array(ptr, count, size) memtrack_realloc_array((ptr), (count), (size), __FILE__, __LINE__)
#define recalloc_array(ptr, count, size) memtrack_recalloc_array((ptr), (count), (size), __FILE__, __LINE__)
#define get_size(ptr) memtrack_get_size((ptr), __FILE__, __LINE__)



/*
 * memtrack_malloc
 * @param size: size of memory to allocate, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 */
extern void* memtrack_malloc (size_t size, const char* file, int line);

/*
 * memtrack_calloc
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 */
extern void* memtrack_calloc (size_t count, size_t size, const char* file, int line);

/*
 * memtrack_realloc
 * @param ptr: pointer to the memory block to reallocate
 * @param size: new size of the memory block in bytes, the memory block is freed and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 */
extern void* memtrack_realloc (void* ptr, size_t size, const char* file, int line);

/*
 * memtrack_free
 * @param ptr: pointer to the memory block to free
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 */
extern void memtrack_free (void* ptr, const char* file, int line);


/*
 * memtrack_recalloc
 * @param ptr: pointer to the memory block to reallocate
 * @param count: new number of elements to allocate, the memory block is freed and returns NULL if count is 0
 * @param size: new size of each element in bytes, the memory block is freed and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 * @note: newly allocated memory beyond the original size is zero-initialized
 */
extern void* memtrack_recalloc (void* ptr, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_malloc_array
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 */
extern void* memtrack_malloc_array (size_t count, size_t size, const char* file, int line);

/*
 * memtrack_calloc_array
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 * @note: this function is a wrapper for memtrack_calloc
 */
extern void* memtrack_calloc_array (size_t count, size_t size, const char* file, int line);

/*
 * memtrack_realloc_array
 * @param ptr: pointer to the memory block to reallocate
 * @param count: new number of elements to allocate, the memory block is freed and returns NULL if count is 0
 * @param size: new size of each element in bytes, the memory block is freed and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 */
extern void* memtrack_realloc_array (void* ptr, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_recalloc_array
 * @param ptr: pointer to the memory block to reallocate
 * @param count: new number of elements to allocate, the memory block is freed and returns NULL if count is 0
 * @param size: new size of each element in bytes, the memory block is freed and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 * @note: this function is a wrapper for memtrack_recalloc
 */
extern void* memtrack_recalloc_array (void* ptr, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_get_size
 * @param ptr: pointer to the memory block whose size is to be retrieved, displays a message and returns 0 if ptr is NULL
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: size of the memory block in bytes, or 0 on failure
 */
extern size_t memtrack_get_size (void* ptr, const char* file, int line);

/*
 * memtrack_all_check
 * @note: use the printf function to output all information stored in the memory management hashtable during runtime
 */
extern void memtrack_all_check (void);



/*
 * If you want to manipulate memory tracking entries, you can use the functions below,
 * but it's not recommended unless you're creating a wrapper for a function that needs
 * to be freed with free().
 */


/*
 * You must not use functions declared before this lock/unlock function within the
 * lock/unlock block.
 *
 * For functions declared after this one, it is recommended to wrap their usage—along
 * with related logic—within the lock/unlock block.
 */

/*
 * memtrack_lock
 * @note: this function locks the memory tracking system to prevent concurrent access
 */
extern void memtrack_lock (void);

/*
 * memtrack_unlock
 * @note: this function unlocks the memory tracking system to allow access
 */
extern void memtrack_unlock (void);


/*
 * memtrack_entry_add
 * @param ptr: pointer to the memory to add, nothing is done except a warning if NULL
 * @param size: size of the memory to add, no value check is performed
 * @param file: name of the file calling the wrapper function that calls this function
 * @param line: line number of the point where the wrapper function calling this function is invoked
 */
extern void memtrack_entry_add (void* ptr, size_t size, const char* file, int line);

/*
 * memtrack_entry_update
 * @param old_ptr: pointer to the memory before update, execute memtrack_entry_add if NULL
 * @param new_ptr: pointer to the memory after update, the value of old_ptr is used if NULL
 * @param new_size: size of the memory after update, no value check is performed
 * @param file: name of the file calling the wrapper function that calls this function
 * @param line: line number of the point where the wrapper function calling this function is invoked
 */
extern void memtrack_entry_update (void* old_ptr, void* new_ptr, size_t new_size, const char* file, int line);

/*
 * memtrack_entry_free
 * @param ptr: pointer to the memory to free, no action is taken if NULL
 * @param file: name of the file calling the wrapper function that calls this function
 * @param line: line number of the point where the wrapper function calling this function is invoked
 */
extern void memtrack_entry_free (void* ptr, const char* file, int line);


/*
 * The following functions must not be used outside of a lock. When working outside the
 * lock, only use functions whose names do not end with _without_lock.
 */

/*
 * memtrack_malloc_without_lock
 * @param size: size of memory to allocate, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 */
extern void* memtrack_malloc_without_lock (size_t size, const char* file, int line);

/*
 * memtrack_calloc_without_lock
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 */
extern void* memtrack_calloc_without_lock (size_t count, size_t size, const char* file, int line);

/*
 * memtrack_realloc_without_lock
 * @param ptr: pointer to the memory block to reallocate
 * @param size: new size of the memory block in bytes, the memory block is freed and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 */
extern void* memtrack_realloc_without_lock (void* ptr, size_t size, const char* file, int line);

/*
 * memtrack_free_without_lock
 * @param ptr: pointer to the memory block to free
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 */
extern void memtrack_free_without_lock (void* ptr, const char* file, int line);

/*
 * memtrack_recalloc_without_lock
 * @param ptr: pointer to the memory block to reallocate
 * @param count: new number of elements to allocate, the memory block is freed and returns NULL if count is 0
 * @param size: new size of each element in bytes, the memory block is freed and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 * @note: newly allocated memory beyond the original size is zero-initialized
 */
extern void* memtrack_recalloc_without_lock (void* ptr, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_malloc_array_without_lock
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to allocated memory block, or NULL on failure
 */
extern void* memtrack_malloc_array_without_lock (size_t count, size_t size, const char* file, int line);

/*
 * memtrack_realloc_array_without_lock
 * @param ptr: pointer to the memory block to reallocate
 * @param count: new number of elements to allocate, the memory block is freed and returns NULL if count is 0
 * @param size: new size of each element in bytes, the memory block is freed and returns NULL if size is 0
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: pointer to reallocated memory block, or NULL on failure
 */
extern void* memtrack_realloc_array_without_lock (void* ptr, size_t count, size_t size, const char* file, int line);

/*
 * memtrack_get_size_without_lock
 * @param ptr: pointer to the memory block whose size is to be retrieved, displays a message and returns 0 if ptr is NULL
 * @param file: name of the calling file, usually specified with __FILE__
 * @param line: line number of the caller, usually specified with __LINE__
 * @return: size of the memory block in bytes, or 0 on failure
 */
extern size_t memtrack_get_size_without_lock (void* ptr, const char* file, int line);


#else  /* defined MEMTRACK_DISABLE */


/*
 * IMPORTANT:
 * The functions recalloc and recalloc_array are not supported in the no-tracking build.
 */


/*
 * malloc_array
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @return: pointer to allocated memory block, or NULL on failure
 */
extern void* malloc_array (size_t count, size_t size);

/*
 * calloc_array
 * @param count: number of elements to allocate, displays a message and returns NULL if count is 0
 * @param size: size of each element in bytes, displays a message and returns NULL if size is 0
 * @return: pointer to allocated memory block, or NULL on failure
 * @note: this function is a wrapper for calloc
 */
extern void* calloc_array (size_t count, size_t size);

/*
 * realloc_array
 * @param ptr: pointer to the memory block to reallocate
 * @param count: new number of elements to allocate, the memory block is freed and returns NULL if count is 0
 * @param size: new size of each element in bytes, the memory block is freed and returns NULL if size is 0
 * @return: pointer to reallocated memory block, or NULL on failure
 */
extern void* realloc_array (void* ptr, size_t count, size_t size);


#endif


MHT_CPP_C_END


#endif
