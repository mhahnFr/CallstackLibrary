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

#include "../objectFile.h"

struct objectFile_private {
    struct objectFile _;
    
    struct function * functions;
};

struct objectFile * objectFile_new(void) {
    struct objectFile_private * self = malloc(sizeof(struct objectFile_private));
    if (self == NULL) {
        return NULL;
    }
    objectFile_create(&self->_);
    self->_.priv = self;
    return &self->_;
}

void objectFile_addFunction(struct objectFile * me,
                            struct function *   function) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;
    
    function->next  = self->functions;
    self->functions = function;
}

int64_t objectFile_findClosestFunction(struct objectFile * me, uint64_t address,
                                       struct function **  funcPtr) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;
    
    struct function * func = NULL;
    int64_t distance = INT64_MAX;
    for (struct function * it = self->functions; it != NULL; it = it->next) {
        int64_t diff = address - it->startAddress;
        if (diff < distance && diff >= 0) {
            distance = diff;
            func = it;
        }
    }
    *funcPtr = func;
    return distance;
}

void objectFile_destroy(struct objectFile * me) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;
    
    for (struct function * tmp = self->functions; tmp != NULL;) {
        struct function * n = tmp->next;
        function_delete(tmp);
        tmp = n;
    }
}

void objectFile_delete(struct objectFile * self) {
    objectFile_destroy(self);
    free(self->priv);
}
