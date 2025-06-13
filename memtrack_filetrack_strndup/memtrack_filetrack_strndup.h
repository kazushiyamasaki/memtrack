/*
 * memtrack_filetrack_strndup.h -- interface of a library that adds filetrack_strndup to
 *                                 the management target of memtrack.h
 * version 0.9.0, June 13, 2025
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

#ifndef MEMTRACK_FILETRACK_STRNDUP_H
#define MEMTRACK_FILETRACK_STRNDUP_H


#ifndef MEMTRACK_DISABLE


#include "filetrack.h"
#include "memtrack.h"


MHT_CPP_C_BEGIN


/*
 * Replaces filetrack_strndup with memory-tracking versions.
 * To disable this behavior, define MEMTRACK_DISABLE_REPLACE_FILETRACK_FUNC
 * macro before including this file.
 */
#ifndef MEMTRACK_DISABLE_REPLACE_FILETRACK_FUNC
	#define filetrack_strndup(string, max_bytes) memtrack_filetrack_strndup((string), (max_bytes), __FILE__, __LINE__)
#endif


/*
 * memtrack_filetrack_strndup
 * @param string: the string to duplicate
 * @param max_bytes: the maximum number of bytes to copy from the string
 * @return: a newly allocated string that is a duplicate of the input string, or NULL on failure
 * @note: the strings duplicated by this function must be freed with free()
 */
extern char* memtrack_filetrack_strndup (const char* string, size_t max_bytes, const char* file, int line);


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
 * memtrack_filetrack_strndup_without_lock
 * @param string: the string to duplicate
 * @param max_bytes: the maximum number of bytes to copy from the string
 * @return: a newly allocated string that is a duplicate of the input string, or NULL on failure
 * @note: the strings duplicated by this function must be freed with free()
 */
extern char* memtrack_filetrack_strndup_without_lock (const char* string, size_t max_bytes, const char* file, int line);


MHT_CPP_C_END


#endif


#endif
