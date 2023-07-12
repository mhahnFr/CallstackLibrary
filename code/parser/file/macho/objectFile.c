/*
 * Callstack Library - A library creating human readable call stacks.
 *
 * Copyright (C) 2023  mhahnFr
 *
 * This file is part of the CallstackLibrary. This library is free software:
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include "objectFile.h"

struct objectFile * objectFile_new(void) {
    struct objectFile * toReturn = malloc(sizeof(struct objectFile));
    if (toReturn != NULL) {
        objectFile_create(toReturn);
    }
    return toReturn;
}

void objectFile_create(struct objectFile * self) {
    self->sourceFile = NULL;
    self->directory  = NULL;
    self->name       = NULL;
    self->functions  = NULL;
    self->next       = NULL;
}

void objectFile_destroy(struct objectFile * self) {
    // TODO: Implement
    (void) self;
}

void objectFile_delete(struct objectFile * self) {
    objectFile_destroy(self);
    free(self);
}
