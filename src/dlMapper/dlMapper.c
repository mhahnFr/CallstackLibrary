/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2026  mhahnFr
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

#include "dlMapper.h"

#include <stdlib.h>

#include "dlMapper_platform.h"

/** The loaded library infos.                            */
static vector_binaryFile_t loadedLibs = vector_initializer;
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
static inline int dlMapper_sortCompare(const struct binaryFile** lhs, const struct binaryFile** rhs) {
    if ((*lhs)->startAddress < (*rhs)->startAddress) return -1;
    if ((*lhs)->startAddress > (*rhs)->startAddress) return +1;
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

bool dlMapper_isInited(void) {
    return dlMapper_inited;
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
static inline int dlMapper_searchCompare(const void* key, const struct binaryFile** element) {
    // IMPORTANT: key is the searched address, element the array element

    const struct binaryFile* file = *element;
    if (key >= file->startAddress && key < file->end) {
        return 0;
    }
    return key > file->startAddress ? +1 : -1;
}

static inline int dlMapper_searchCompareRegion(const void* key, const pair_ptr_t* element) {
    const uintptr_t k = (uintptr_t) key;
    if (k >= element->first && k < element->second) {
        return 0;
    }
    return k > element->first ? +1 : -1;
}

struct binaryFile* dlMapper_binaryFileForAddress(const void* address, const bool includeRegions) {
    if (!dlMapper_inited) return NULL;

    struct binaryFile** toReturn = vector_search(&loadedLibs, address, dlMapper_searchCompare);
    if (toReturn == NULL && includeRegions) {
        vector_iterate(&loadedLibs, {
            const pair_ptr_t* result = vector_search(binaryFile_getRegions(*element),
                                                     address, dlMapper_searchCompareRegion);
            if (result != NULL) {
                return *element;
            }
        });
    }
    return toReturn == NULL ? NULL : *toReturn;
}

struct binaryFile* dlMapper_binaryFileForFileName(const char* fileName) {
    if (!dlMapper_inited) return NULL;

    vector_iterate(&loadedLibs, {
        struct binaryFile* theElement = *element;
        if (strcmp(fileName, theElement->fileName.original) == 0
            || strcmp(fileName, theElement->fileName.absolute) == 0
            || strcmp(fileName, theElement->fileName.relative) == 0) {
            return theElement;
        }
    });
    return NULL;
}

pair_relativeFile_t dlMapper_relativize(const void* address) {
    struct binaryFile* file = dlMapper_binaryFileForAddress(address, false);
    return (pair_relativeFile_t) {
        file,
        file == NULL ? 0 : dlMapper_platform_relativize(file, address)
    };
}

void* dlMapper_absolutize(const void* address, const char* binaryName) {
    const struct binaryFile* info = dlMapper_binaryFileForFileName(binaryName);
    if (info == NULL) {
        return NULL;
    }
    return (void*) dlMapper_platform_absolutize(info, (uintptr_t) address);
}

const vector_binaryFile_t* dlMapper_getLoadedBinaries(void) {
    return &loadedLibs;
}

void dlMapper_deinit(void) {
    vector_destroyWith(&loadedLibs, binaryFile_delete);
    vector_init(&loadedLibs);
    dlMapper_inited = false;
}
