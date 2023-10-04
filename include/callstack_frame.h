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

#ifndef callstack_frame_h
#define callstack_frame_h

#include <stdlib.h>

#include "optional_Dl_info.h"

struct callstack_frame {
    optional_Dl_info_t info;
    
    char * binaryFile;
    char * function;
    char * sourceFile;
    unsigned long sourceLine;
};

static inline void callstack_frame_create(struct callstack_frame * self) {
    self->binaryFile = NULL;
    self->function   = NULL;
    self->sourceFile = NULL;
    self->sourceLine = 0;
    
    self->info = (optional_Dl_info_t) { .has_value = false };
}

static inline struct callstack_frame * callstack_frame_new(void) {
    struct callstack_frame * toReturn = (struct callstack_frame *) malloc(sizeof(struct callstack_frame));
    
    if (toReturn != NULL) {
        callstack_frame_create(toReturn);
    }
    
    return toReturn;
}

struct callstack_frame * callstack_frame_copy(struct callstack_frame * self);

void callstack_frame_copyHere(struct callstack_frame * destination, const struct callstack_frame * source);

static inline void callstack_frame_destroy(struct callstack_frame * self) {
    free(self->binaryFile);
    free(self->function);
    free(self->sourceFile);
}

static inline void callstack_frame_delete(struct callstack_frame * self) {
    callstack_frame_destroy(self);
    free(self);
}

#endif /* callstack_frame_h */
