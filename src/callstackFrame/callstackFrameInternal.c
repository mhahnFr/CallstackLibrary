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

#include "callstackFrameInternal.h"

#include "../dlMapper/dlMapper.h"

void callstackFrame_translateBinary(struct callstack_frame* self, const void* address,
                                    const bool useCache, const bool includeRegions) {
    *self = callstack_frame_initializer;

    struct loadedLibInfo* info = dlMapper_libInfoForAddress(address, includeRegions);
    if (info != NULL) {
        self->binaryFile = useCache ? info->absoluteFileName : strdup(info->absoluteFileName);
        self->binaryFileRelative = useCache ? info->relativeFileName : strdup(info->relativeFileName);
        self->binaryFileIsSelf = info->isSelf;
    }
    self->reserved = info;
    self->reserved1 = useCache;
}
