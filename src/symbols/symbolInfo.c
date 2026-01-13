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

#include "../callstackFrame/callstackFrameInternal.h"
#include "../dlMapper/dlMapper.h"

static inline struct callstack_frame symbols_getInfoShared(const void* address, const bool useCache) {
    struct callstack_frame toReturn;
    dlMapper_init();
    // FIXME: + 1 byte in order to bypass symbol table usage for functions.   - mhahnFr
    const void* searchAddress = address + 1;
    callstackFrame_translateBinary(&toReturn, searchAddress, useCache, true);
    struct binaryFile* file = toReturn.reserved;
    // TODO: if null or if failed to parse
    binaryFile_addr2String(file, searchAddress, &toReturn);
    return toReturn;
}

struct callstack_frame symbols_getInfo(const void* address) {
    return symbols_getInfoShared(address, false);
}

struct callstack_frame symbols_getInfoCached(const void* address) {
    return symbols_getInfoShared(address, true);
}
