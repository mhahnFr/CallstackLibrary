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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef CXX_FUNCTIONS
# include "fileHelper.h"
#endif

/**
 * Duplicates or converts the given path to an absolute path.
 *
 * @param path the path to be converted
 * @param f whether to free the given path name
 * @return the allocated, converted path name
 */
static inline char* path_toAbsolutePathIntern(char* path, const bool f) {
    char* toReturn;
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

/**
 * Duplicates or converts the given path to a relative path.
 *
 * @param path the path to be converted
 * @param f whether to free the given path name
 * @return the allocated, converted path name
 */
static inline char* path_toRelativePathIntern(char* path, const bool f) {
    char* toReturn;
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

char* path_toRelativePath(const char* path) {
    return path_toRelativePathIntern((char*) path, false);
}

char* path_toRelativePathFree(char* path) {
    return path_toRelativePathIntern(path, true);
}

char* path_toAbsolutePath(const char* path) {
    return path_toAbsolutePathIntern((char*) path, false);
}

char* path_toAbsolutePathFree(char* path) {
    return path_toAbsolutePathIntern(path, true);
}
