/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
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

struct functionInfo functionInfo_loadHint(const char* functionName, const char* libraryName) {
    struct functionInfo toReturn = (struct functionInfo) { 0, 0 };

    dlMapper_init();
    bool found = false;
    if (libraryName != NULL) {
        struct loadedLibInfo* info = dlMapper_libInfoForFileName(libraryName);
        if (info != NULL) {
            if (info->associated == NULL) {
                info->associated = binaryFile_new(info->fileName, info->begin);
            }
            struct binaryFile* file = info->associated;
            if (file != NULL) {
                file->relocationOffset = info->relocationOffset;
                file->inMemory = true;
                found = file->getFunctionInfo(file, functionName, &toReturn);
            }
        }
    }
    if (!found) {
        // TODO: Search in all known binaries
    }

    if (callstack_autoClearCaches) {
        callstack_clearCaches();
    }
    return toReturn;
}
