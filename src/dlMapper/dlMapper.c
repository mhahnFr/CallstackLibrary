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

#include "dlMapper.h"
#include "dlMapper_platform.h"
#include "vector_loadedLibInfo.h"

static vector_loadedLibInfo_t loadedLibs = vector_initializer;
static bool dlMapper_inited = false;

bool dlMapper_init(void) {
    if (dlMapper_inited) return true;

    const bool result = dlMapper_platform_loadLoadedLibraries(&loadedLibs);
    if (!result) {
        dlMapper_deinit();
    }
    dlMapper_inited = result;
    return result;
}

const char* dlMapper_fileNameForAddress(const void* address) {
    if (!dlMapper_inited) return NULL;

    vector_iterate(struct loadedLibInfo, &loadedLibs, {
        if (address >= element->begin && address < element->end) {
            return element->fileName;
        }
    })
    return NULL;
}

void dlMapper_deinit(void) {
    vector_loadedLibInfo_destroyWithPtr(&loadedLibs, loadedLibInfo_destroy);
    vector_loadedLibInfo_create(&loadedLibs);
    dlMapper_inited = false;
}
