/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023  mhahnFr
 *
 * This file is part of the CallstackLibrary. This library is free software:
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef cache_h
#define cache_h

#include "../binaryFile.h"

/**
 * @brief Returns a binary file structure used to parse the binary
 * file pointed to by the given file name.
 *
 * If the given cache is `NULL`, the global cache is used.
 *
 * The given cache is searched for the file representation, if it is not
 * found, it is created and added to the given cache.
 *
 * If no file representation exists in the given cache and creation fails,
 * `NULL` is returned.
 *
 * @param cache the cache to be used
 * @param fileName the name of the file
 * @return the binary file structure representation
 */
struct binaryFile * cache_findOrAddFile(struct binaryFile ** cache, const char * fileName);

/**
 * @brief Clears the given cache.
 *
 * Passing `NULL`, the global cache is cleared.
 *
 * @param cache the cache to be cleared
 */
void cache_clear(struct binaryFile ** cache);

#endif /* cache_h */
