/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2025 - 2026  mhahnFr
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

#include <symbols/symbolInfo.h>

#include "callstack_internals.h"
#include "../callstackFrame/callstackFrameInternal.h"
#include "../dlMapper/dlMapper.h"
#include "misc/cache.h"

static inline SymbolInfo symbols_getInfoShared(const void* address, bool* success, const bool useCache) {
    SymbolInfo toReturn;
    dlMapper_init();
    callstackFrame_translateBinary(&toReturn, address, useCache, true);
    struct binaryFile* file = toReturn.reserved;
    bool succeeded = false;
    if (file != NULL) {
        succeeded = binaryFile_getSymbolInfo(file, address, &toReturn);
    }
    if (success != NULL) *success = succeeded;
    maybeV(callstack_clearCaches);
    return toReturn;
}

SymbolInfo symbols_getInfo(const void* address, bool* success) {
    return symbols_getInfoShared(address, success, false);
}

SymbolInfo symbols_getInfoCached(const void* address, bool* success) {
    return symbols_getInfoShared(address, success, true);
}

void symbols_destroyInfo(const SymbolInfo* info) {
    callstack_frame_destroy(info);
}