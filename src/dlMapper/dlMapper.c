/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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
#include "vector_loadedLibInfo.h"

/** The loaded library infos.                            */
static vector_loadedLibInfo_t loadedLibs = vector_initializer;
/** Indicates whether the dlMapper has been initialized. */
static bool dlMapper_inited = false;

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
        qsort(loadedLibs.content, loadedLibs.count, sizeof(struct loadedLibInfo), dlMapper_sortCompare);
    }
    dlMapper_inited = result;
    return result;
}

/**
 * @brief Returns how the given key compares to the given loaded library info.
 *
 * The given key is the searched address, the given element is a loaded library info object.
 *
 * @param key the searched key
 * @param element the element to be checked
 * @return `0` if the key is in the loaded library or a value smaller or greater than `0` according to the sorting order
 */
static inline int dlMapper_searchCompare(const void* key, const void* element) {
    // IMPORTANT: key is the searched address, element the array element

    struct loadedLibInfo* e = (struct loadedLibInfo*) element;
    if (key >= e->begin && key < e->end) {
        return 0;
    }
    return key > e->begin ? +1 : -1;
}

struct loadedLibInfo* dlMapper_libInfoForAddress(const void* address) {
    if (!dlMapper_inited) return NULL;

    return bsearch(address, loadedLibs.content, loadedLibs.count, sizeof(struct loadedLibInfo), dlMapper_searchCompare);
}

void dlMapper_deinit(void) {
    vector_loadedLibInfo_destroyWithPtr(&loadedLibs, loadedLibInfo_destroy);
    vector_loadedLibInfo_create(&loadedLibs);
    dlMapper_inited = false;
}
