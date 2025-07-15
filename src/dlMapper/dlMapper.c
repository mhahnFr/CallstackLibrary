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

#include <stdlib.h>

#include "dlMapper.h"
#include "dlMapper_platform.h"

/** The loaded library infos.                            */
static vector_loadedLibInfo_t loadedLibs = vector_initializer;
/** Indicates whether the dlMapper has been initialized. */
static bool dlMapper_inited = false;

/**
 * @brief Returns how the two given loaded library infos compare.
 *
 * Sorted ascendingly.
 *
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return @c 0 if the two values compare equal or a value less than or greater
 * than @c 0 according to the sorting order
 */
static inline int dlMapper_sortCompare(const void* lhs, const void* rhs) {
    const struct loadedLibInfo* a = lhs,
                              * b = rhs;

    if (a->begin < b->begin) return -1;
    if (a->begin > b->begin) return +1;

    return 0;
}

bool dlMapper_init(void) {
    if (dlMapper_inited) return true;

    const bool result = dlMapper_platform_loadLoadedLibraries(&loadedLibs);
    if (!result) {
        dlMapper_deinit();
    } else {
        vector_sort(&loadedLibs, dlMapper_sortCompare);
    }
    return dlMapper_inited = result;
}

/**
 * @brief Returns how the given key compares to the given loaded library info.
 *
 * The given key is the searched address, the given element is a loaded library
 * info object.
 *
 * @param key the searched key
 * @param element the element to be checked
 * @return @c 0 if the key is in the loaded library or a value smaller or
 * greater than @c 0 according to the sorting order
 */
static inline int dlMapper_searchCompare(const void* key, const void* element) {
    // IMPORTANT: key is the searched address, element the array element

    const struct loadedLibInfo* e = (struct loadedLibInfo*) element;
    if (key >= e->begin && key < e->end) {
        return 0;
    }
    return key > e->begin ? +1 : -1;
}

struct loadedLibInfo* dlMapper_libInfoForAddress(const void* address) {
    if (!dlMapper_inited) return NULL;

    return vector_search(&loadedLibs, address, dlMapper_searchCompare);
}

struct loadedLibInfo* dlMapper_libInfoForFileName(const char* fileName) {
    if (!dlMapper_inited) return NULL;

    vector_iterate(&loadedLibs, {
        if (strcmp(fileName, element->fileName) == 0
            || strcmp(fileName, element->absoluteFileName) == 0
            || strcmp(fileName, element->relativeFileName) == 0) {
            return element;
        }
    });
    return NULL;
}

const vector_loadedLibInfo_t* dlMapper_getLoadedLibraries(void) {
    return &loadedLibs;
}

void dlMapper_deinit(void) {
    vector_destroyWithPtr(&loadedLibs, loadedLibInfo_destroy);
    vector_init(&loadedLibs);
    dlMapper_inited = false;
}
