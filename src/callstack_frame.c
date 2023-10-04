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
    
    if (toReturn != NULL) {
        callstack_frame_copyHere(toReturn, self);
    }
    
    return toReturn;
}

static inline char * maybeStrdup(const char * str) {
    return str == NULL ? NULL : strdup(str);
}

void callstack_frame_copyHere(struct callstack_frame * destination, const struct callstack_frame * source) {
    destination->binaryFile = maybeStrdup(source->binaryFile);
    destination->function   = maybeStrdup(source->function);
    destination->sourceFile = maybeStrdup(source->sourceFile);
    destination->sourceLine = source->sourceLine;
}
