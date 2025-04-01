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

#include <callstack_internals.h>
#include <functionInfo/functionInfo.h>

#include "../dlMapper/dlMapper.h"

/**
 * Fills the given function info structure using the given loaded runtime image info.
 *
 * @param info the runtime image info to be examined
 * @param functionName the name of the function to be found
 * @param functionInfo the function info structure to be filled
 * @return whether the structure was filled, e. g. whether the function was found in the runtime image
 */
static inline bool functionInfo_getFrom(struct loadedLibInfo* info, const char* functionName, struct functionInfo* functionInfo) {
    if (info == NULL) return false;

    if (info->associated == NULL) {
        info->associated = binaryFile_new(info->fileName, info->begin);
    }
    struct binaryFile* file = info->associated;
    if (file == NULL) return false;

    file->relocationOffset = info->relocationOffset;
    file->inMemory = true;
    return functionInfo->found = binaryFile_getFunctionInfo(file, functionName, functionInfo);
}

struct functionInfo functionInfo_loadHint(const char* functionName, const char* libraryName) {
    struct functionInfo toReturn = (struct functionInfo) { 0, 0, false };

    dlMapper_init();
    if (libraryName != NULL && functionInfo_getFrom(dlMapper_libInfoForFileName(libraryName), functionName, &toReturn)) {
        if (callstack_autoClearCaches) {
            callstack_clearCaches();
        }
        return toReturn;
    }

    vector_iterate(dlMapper_getLoadedLibraries(), {
        if (functionInfo_getFrom(element, functionName, &toReturn)) {
            break;
        }
    });

    if (callstack_autoClearCaches) {
        callstack_clearCaches();
    }
    return toReturn;
}
