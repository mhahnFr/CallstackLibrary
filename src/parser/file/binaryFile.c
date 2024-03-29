/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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

#include "binaryFile.h"

#include "fileHelper.h"

#ifdef __APPLE__
 #include "macho/machoFile.h"
 #define LCS_MACHO
#elif defined(__linux__)
 #include "elf/elfFile.h"
 #define LCS_ELF
#endif

/** A list of the already parsed files. */
static struct binaryFile* parsedFiles = NULL;

struct binaryFile * binaryFile_new(const char * fileName, void* startAddress) {
    struct binaryFile * toReturn;
    
#ifdef LCS_MACHO
    struct machoFile * tmp = machoFile_new(fileName);
    toReturn = tmp == NULL ? NULL : &tmp->_;
#elif defined(LCS_ELF)
    struct elfFile * tmp = elfFile_new();
    toReturn = tmp == NULL ? NULL : &tmp->_;
#else
    toReturn = NULL;
#endif
    
    if (toReturn != NULL) {
        toReturn->fileName     = fileName;
        toReturn->startAddress = startAddress;
    }
    return toReturn;
}

void binaryFile_create(struct binaryFile * self) {
    self->fileName = NULL;
    self->next     = NULL;
    self->parsed   = false;
}

/**
 * @brief Returns an allocated copy of the given path.
 *
 * If the C++ functionality of this library is enabled, the returned path is
 * made relative to the current working directory.
 *
 * @param path the path to be copied
 * @param f whether to free the given path after copying
 * @return the allocated copy
 */
static inline char * binaryFile_toRelativePathIntern(char * path, bool f) {
    char * toReturn;
#ifdef CXX_FUNCTIONS
    toReturn = lcs_toRelativePath(path);
#else
    toReturn = strdup(path);
#endif
    if (f) {
        free(path);
    }
    return toReturn;
}

char * binaryFile_toRelativePath(char * path) {
    return binaryFile_toRelativePathIntern(path, false);
}

char * binaryFile_toRelativePathFree(char * path) {
    return binaryFile_toRelativePathIntern(path, true);
}

/**
 * @brief Returns an allocated copy of the given path.
 *
 * If the C++ functionality of this library is enabled, the copy is converted
 * to an absolute path.
 *
 * @param path the path to be copied
 * @param f whether to free the given path after copying
 * @return the allocated copy
 */
static inline char * binaryFile_toAbsolutePathIntern(char * path, bool f) {
    char * toReturn;
#ifdef CXX_FUNCTIONS
    toReturn = lcs_toCanonicalPath(path);
#else
    toReturn = strdup(path);
#endif
    if (f) {
        free(path);
    }
    return toReturn;
}

char * binaryFile_toAbsolutePath(char * path) {
    return binaryFile_toAbsolutePathIntern(path, false);
}

char * binaryFile_toAbsolutePathFree(char * path) {
    return binaryFile_toAbsolutePathIntern(path, true);
}

struct binaryFile* binaryFile_findOrAddFile(const char* fileName, void* startAddress) {
    struct binaryFile* it;
    for (it = parsedFiles; it != NULL && strcmp(it->fileName, fileName) != 0; it = it->next);
    
    if (it == NULL) {
        it = binaryFile_new(fileName, startAddress);
        if (it == NULL) {
            return NULL;
        }
        it->next    = parsedFiles;
        parsedFiles = it;
    }
    return it;
}

void binaryFile_clearCaches(void) {
#ifdef LCS_MACHO
    machoFile_clearCaches();
#endif
    
    struct binaryFile* tmp;
    
    for (struct binaryFile* it = parsedFiles; it != NULL; it = tmp) {
        tmp = it->next;
        it->deleter(it);
    }
    parsedFiles = NULL;
}
