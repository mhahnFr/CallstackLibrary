/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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

#include <stdbool.h>
#include <string.h>

#include "cache.h"
#include <secure/_string.h>

static struct macho_cache {
    struct objectFile* objectFiles;
} cache = {
    NULL
};

static inline bool macho_cache_isInArchive(const char* fileName) {
    if (fileName == NULL) return false;
    
    char* lp = strrchr(fileName, '(');
    char* rp = strrchr(fileName, ')');
    
    return lp != NULL && rp != NULL && lp < rp;
}

static inline char* macho_cache_getArchiveName(const char* fileName) {
    if (fileName == NULL) return NULL;
    
    const char* lp = strrchr(fileName, '(');
    if (lp == NULL) return NULL;
    
    const size_t size = lp - fileName + 1;
    char* toReturn = malloc(size);
    if (toReturn == NULL) return NULL;
    
    strlcpy(toReturn, fileName, size);
    toReturn[size - 1] = '\0';
    return toReturn;
}

static inline bool macho_cache_loadArchiveFree(char* archiveName) {
    // TODO: Implement
    
    free(archiveName);
    return false;
}

struct objectFile* macho_cache_findOrAdd(char* fileName) {
    struct objectFile* it;
    for (it = cache.objectFiles; it != NULL && strcmp(it->name, fileName) != 0; it = it->next);
    
    if (it == NULL) {
        if (macho_cache_isInArchive(fileName)) {
            char* archiveName = macho_cache_getArchiveName(fileName);
            if (macho_cache_loadArchiveFree(archiveName)) {
                return macho_cache_findOrAdd(fileName);
            }
        }
        
        it = objectFile_new();
        if (it == NULL) {
            return NULL;
        }
        it->name = fileName;
        it->next = cache.objectFiles;
        cache.objectFiles = it;
    }
    return it;
}

void macho_cache_delete(struct objectFile* file) {
    if (cache.objectFiles == file) {
        cache.objectFiles = cache.objectFiles->next;
    } else {
        struct objectFile* prev;
        for (prev = cache.objectFiles; prev != NULL && prev->next != file; prev = prev->next);
        
        if (prev != NULL) {
            prev->next = file->next;
        }
    }
    objectFile_delete(file);
}

void macho_cache_destroy(void) {
    struct objectFile* tmp;
    for (struct objectFile* it = cache.objectFiles; it != NULL; it = tmp) {
        tmp = it->next;
        objectFile_delete(it);
    }
    cache.objectFiles = NULL;
}
