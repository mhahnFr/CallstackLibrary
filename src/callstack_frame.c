/*
 * Callstack Library - Library creating human-readable call stacks.
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

#include <string.h>

#include "../include/callstack_frame.h"

struct callstack_frame * callstack_frame_copy(struct callstack_frame * self) {
    struct callstack_frame * toReturn = callstack_frame_new();
    
    if (toReturn == NULL) {
        return NULL;
    }
    
    if (self->function != NULL) {
        toReturn->function = strdup(self->function);
    }
    if (self->binaryFile != NULL) {
        toReturn->binaryFile = strdup(self->binaryFile);
    }
    if (self->sourceFile != NULL) {
        toReturn->sourceFile = strdup(self->sourceFile);
    }
    toReturn->sourceLine = self->sourceLine;
    
    return toReturn;
}
