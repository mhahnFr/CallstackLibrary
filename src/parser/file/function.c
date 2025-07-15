/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
 *
 * This file is part of the CallstackLibrary.
 *
 * The CallstackLibrary is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
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

#include "function.h"

struct function * function_new(void) {
    struct function * toReturn = malloc(sizeof(struct function));
    if (toReturn != NULL) {
        function_create(toReturn);
    }
    return toReturn;
}

void function_create(struct function * self) {
    self->startAddress  = 0x0;
    self->length        = 0x0;
    self->linkedName    = NULL;
    self->demangledName = (optional_string_t) { .has_value = false };
}

void function_destroy(const struct function* self) {
    free(self->linkedName);
    if (self->demangledName.has_value) {
        free(self->demangledName.value);
    }
}

void function_delete(struct function * self) {
    function_destroy(self);
    free(self);
}
