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
    self->startAddress = 0x0;
    self->endAddress   = 0x0;
    self->linkedName   = NULL;
}

void function_destroy(struct function * self) {
    free(self->linkedName);
}

void function_delete(struct function * self) {
    function_destroy(self);
    free(self);
}
