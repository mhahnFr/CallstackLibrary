/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2025  mhahnFr
 *
 * This file is part of the CallstackLibrary.
 *
 * The CallstackLibrary is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The CallstackLibrary is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * CallstackLibrary, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef cache_h
#define cache_h

#include <stdint.h>

#include "objectFile.h"

/**
 * Finds or adds the object file object with the given file name and the given
 * timestamp.
 *
 * @param fileName the name of the object file
 * @param lastModified the timestamp of the last modification
 * @return the object file object or @c NULL if unable to allocate
 */
struct objectFile* macho_cache_findOrAdd(const char* fileName, uint64_t lastModified);

/**
 * Deletes the given object file object from the cache.
 *
 * @param file the file to be deleted
 */
void macho_cache_delete(struct objectFile* file);

/**
 * Destroys the whole cache and its contents.
 */
void macho_cache_destroy(void);

#endif /* cache_h */
