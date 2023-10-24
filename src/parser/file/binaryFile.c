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

#include "binaryFile.h"

#include "fileHelper.h"

#ifdef __APPLE__
 #include "macho/machoFile.h"
#elif defined(__linux__)
 #include "elf/elfFile.h"
#endif

struct binaryFile * binaryFile_new(const char * fileName) {
    struct binaryFile * toReturn;
    
#ifdef __APPLE__
    struct machoFile * tmp = machoFile_new();
    toReturn = tmp == NULL ? NULL : &tmp->_;
#elif defined(__linux__)
    struct elfFile * tmp = elfFile_new();
    toReturn = tmp == NULL ? NULL : &tmp->_;
#else
    toReturn = NULL;
#endif
    
    if (toReturn != NULL) {
        toReturn->fileName = fileName;
    }
    return toReturn;
}

void binaryFile_create(struct binaryFile * self) {
    self->fileName = NULL;
    self->next     = NULL;
    self->parsed   = false;
}

static inline char * binaryFile_toRelativePathIntern(char * path, bool f) {
    char * toReturn;
#ifdef CXX_FUNCTIONS
    toReturn = lcs_toRelativePath(path);
    if (f) {
        free(path);
    }
#else
    (void) f;
    toReturn = path;
#endif
    return toReturn;
}

char * binaryFile_toRelativePath(char * path) {
    return binaryFile_toRelativePathIntern(path, false);
}

char * binaryFile_toRelativePathFree(char * path) {
    return binaryFile_toRelativePathIntern(path, true);
}
