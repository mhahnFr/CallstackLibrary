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

#include <mach-o/dyld.h>

#include "archive.h"
#include "cache.h"
#include "vector_string.h"
#include "vector_boolString.h"

static struct macho_cache {
    struct objectFile* objectFiles;
    struct vector_boolString loadedFiles;
    struct vector_string loadedArchives;
} cache = {
    NULL, vector_initializer, vector_initializer
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

static inline void macho_cache_archiveCallback(struct objectFile* file) {
    file->next = cache.objectFiles;
    cache.objectFiles = file;
}

static inline bool macho_cache_loadArchive(const char* archiveName) {
    if (archiveName == NULL) return false;
    
    const bool success = macho_archive_parse(archiveName, macho_cache_archiveCallback);
    return success;
}

static inline void macho_cache_loadLoadedFiles(void) {
    const uint32_t size = _dyld_image_count();
    
    for (uint32_t i = 0; i < size; ++i) {
        vector_boolString_push_back(&cache.loadedFiles, (pair_boolString_t) { true, _dyld_get_image_name(i) });
    }
}

static inline bool macho_cache_archiveLoaded(const char* archiveName) {
    vector_iterate(const char*, &cache.loadedArchives, {
        if (strcmp(archiveName, *element) == 0) {
            return true;
        }
    })
    return false;
}

struct objectFile* macho_cache_findOrAdd(const char* fileName, uint64_t lastModified) {
    struct objectFile* it;
    for (it = cache.objectFiles; it != NULL && strcmp(it->name, fileName) != 0; it = it->next);
    
    if (it == NULL || it->lastModified != lastModified) {
        if (macho_cache_isInArchive(fileName)) {
            char* archiveName = macho_cache_getArchiveName(fileName);
            if (!macho_cache_archiveLoaded(archiveName) && macho_cache_loadArchive(archiveName)) {
                vector_string_push_back(&cache.loadedArchives, archiveName);
                return macho_cache_findOrAdd(fileName, lastModified);
            } else {
                free(archiveName);
            }
        }
        
        it = objectFile_new();
        if (it == NULL) {
            return NULL;
        }
        it->name          = strdup(fileName);
        it->lastModified  = lastModified;
        it->next          = cache.objectFiles;
        cache.objectFiles = it;
    }
    return it;
}

bool macho_cache_isLoaded(struct machoFile* file) {
    if (cache.loadedFiles.count == 0) {
        macho_cache_loadLoadedFiles();
    }
    
    vector_iterate(pair_boolString_t, &cache.loadedFiles, {
        if (strcmp(element->second, file->_.fileName) == 0) {
            return element->first;
        }
    })
    return false;
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
    vector_boolString_destroy(&cache.loadedFiles);
    vector_boolString_create(&cache.loadedFiles);
    vector_iterate(const char*, &cache.loadedArchives, free((void*) *element);)
    vector_string_destroy(&cache.loadedArchives);
    vector_string_create(&cache.loadedArchives);
}
