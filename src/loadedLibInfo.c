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

#include "loadedLibInfo.h"

bool loadedLibInfo_prepare(struct loadedLibInfo* info) {
    if (info == NULL) {
        return false;
    }
    if (info->associated == NULL) {
        info->associated = binaryFile_new(info->fileName, info->begin);
    }
    struct binaryFile* file = info->associated;
    if (file == NULL) {
        return false;
    }
    file->relocationOffset = info->relocationOffset;
    file->inMemory = true;
    return true;
}

void loadedLibInfo_destroy(struct loadedLibInfo* self) {
    free(self->fileName);
    free(self->absoluteFileName);
    free(self->relativeFileName);

    if (self->associated != NULL) {
        binaryFile_delete(self->associated);
    }
}
