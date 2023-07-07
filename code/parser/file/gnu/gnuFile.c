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

#include "gnuFile.h"

struct gnuFile * gnuFile_new(void) {
    struct gnuFile * toReturn = malloc(sizeof(struct gnuFile));
    
    if (toReturn != NULL) {
        gnuFile_create(toReturn);
    }
    return toReturn;
}

void gnuFile_create(struct gnuFile * self) {
    self->_.type     = GNU_FILE;
    self->_.concrete = self;
    self->_.fileName = NULL;
    
    self->_.destroy  = &gnuFile_destroy;
    self->_.delete   = &gnuFile_delete;
}

void gnuFile_destroy(struct binaryFile * self) {
    (void) self;
}

void gnuFile_delete(struct binaryFile * self) {
    self->destroy(self);
    free(self->concrete);
}
