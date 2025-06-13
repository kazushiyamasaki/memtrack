/*
 * memtrack_filetrack_strndup.c -- implementation part of a library that adds
 *                                 filetrack_strndup to the management target of
 *                                 memtrack.h
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
 */

#include "memtrack_filetrack_strndup.h"

#include <string.h>


#if !defined (__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
	#error "This program requires C99 or higher."
#endif


#undef filetrack_strndup


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


char* memtrack_filetrack_strndup_without_lock (const char* string, size_t max_bytes, const char* file, int line) {
	char* result = filetrack_strndup(string, max_bytes);
	if (UNLIKELY(result == NULL)) {
		fprintf(stderr, "Failed to duplicate string.\nFile: %s   Line: %d\n", file, line);
		return result;
	}

	size_t size = strlen(result);
	memtrack_entry_add(result, size, file, line);

	return result;
}


char* memtrack_filetrack_strndup (const char* string, size_t max_bytes, const char* file, int line) {
	memtrack_lock();
	char* result = memtrack_filetrack_strndup_without_lock(string, max_bytes, file, line);
	memtrack_unlock();
	return result;
}
