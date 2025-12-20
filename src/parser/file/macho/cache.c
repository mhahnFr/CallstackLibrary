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

#include "cache.h"

#include <stdbool.h>
#include <string.h>

#include "archive.h"

/**
 * This structure represents the cache for the Mach-O implementation.
 */
static struct macho_cache {
    /** The already loaded object files.                */
    struct objectFile*       objectFiles;
    /** The already loaded archives (static libraries). */
    struct vector_string     loadedArchives;
} cache = {
    NULL, vector_initializer
};

/**
 * @brief Returns whether the given file name is inside an archive.
 *
 * If @c NULL is passed, @c false is returned.
 *
 * @param fileName the file name to be checked
 * @return whether the file name is inside an archive
 */
static inline bool macho_cache_isInArchive(const char* fileName) {
    if (fileName == NULL) return false;

    const char* lp = strrchr(fileName, '(');
    const char* rp = strrchr(fileName, ')');
    
    return lp != NULL && rp != NULL && lp < rp;
}

/**
 * @brief Returns the archive name of the given file name.
 *
 * The returned string is allocated and needs to be freed.
 *
 * @param fileName the full file name
 * @return the archive name or @c NULL if the allocation failed or the given
 * file name is not inside an archive
 */
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

/**
 * The callback function for the archive parser adds the given object file
 * object to the cache.
 *
 * @param file the object file object to be added
 */
static inline void macho_cache_archiveCallback(struct objectFile* file) {
    file->next = cache.objectFiles;
    cache.objectFiles = file;
}

/**
 * @brief Loads the archive of the given file name.
 *
 * When @c NULL is passed, @c false is returned.
 *
 * @param archiveName the file name of the archive to be loaded
 * @return whether the archive was loaded successfully
 */
static inline bool macho_cache_loadArchive(const char* archiveName) {
    if (archiveName == NULL) return false;
    
    return macho_archive_parse(archiveName, macho_cache_archiveCallback);
}

/**
 * Returns whether the given archive was already loaded.
 *
 * @param archiveName the file name of the archive
 * @return whether the archive was already loaded
 */
static inline bool macho_cache_archiveLoaded(const char* archiveName) {
    vector_iterate(&cache.loadedArchives, {
        if (strcmp(archiveName, *element) == 0) {
            return true;
        }
    });
    return false;
}

struct objectFile* macho_cache_findOrAdd(const char* fileName, const uint64_t lastModified) {
    struct objectFile* it;
    for (it = cache.objectFiles; it != NULL && strcmp(it->name, fileName) != 0; it = it->next);
    
    if (it == NULL || it->lastModified != (time_t) lastModified) {
        if (macho_cache_isInArchive(fileName)) {
            char* archiveName = macho_cache_getArchiveName(fileName);
            if (!macho_cache_archiveLoaded(archiveName) && macho_cache_loadArchive(archiveName)) {
                vector_push_back(&cache.loadedArchives, archiveName);
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
        it->lastModified  = (time_t) lastModified;
        it->next          = cache.objectFiles;
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
    vector_destroyWith(&cache.loadedArchives, ((void (*)(const char*)) free));
    vector_init(&cache.loadedArchives);
}
