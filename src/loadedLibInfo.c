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

bool loadedLibInfo_prepare(struct loadedLibInfo* self) {
    if (self == NULL) {
        return false;
    }
    if (self->associated == NULL) {
        self->associated = binaryFile_new(self->fileName, self->begin);
    }
    struct binaryFile* file = self->associated;
    if (file == NULL) {
        return false;
    }
    file->relocationOffset = self->relocationOffset;
    file->inMemory = true;
    return true;
}

void loadedLibInfo_destroy(const struct loadedLibInfo* self) {
    free(self->fileName);
    free(self->absoluteFileName);
    free(self->relativeFileName);

    if (self->associated != NULL) {
        binaryFile_delete(self->associated);
    }
}
