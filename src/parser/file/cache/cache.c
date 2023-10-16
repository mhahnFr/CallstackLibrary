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

#include <stddef.h>
#include <string.h>

#include "cache.h"

/** The global cache. */
struct binaryFile * parsedFiles = NULL;

struct binaryFile * cache_findOrAddFile(struct binaryFile ** cache, const char * fileName) {
    if (cache == NULL) cache = &parsedFiles;
    
    struct binaryFile * it;
    for (it = *cache; it != NULL && strcmp(it->fileName, fileName) != 0; it = it->next);
    
    if (it == NULL) {
        it = binaryFile_new(fileName);
        if (it == NULL) {
            return NULL;
        }
        it->next = *cache;
        *cache   = it;
    }
    return it;
}

void cache_clear(struct binaryFile ** cache) {
    if (cache == NULL) cache = &parsedFiles;
    
    struct binaryFile * tmp;
    
    for (struct binaryFile * it = *cache; it != NULL; it = tmp) {
        tmp = it->next;
        it->delete(it);
    }
    *cache = NULL;
}
