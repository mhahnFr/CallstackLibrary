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
#include "../FunctionVector.h"

/**
 * This structure acts as a wrapper around the object file structure.
 */
struct objectFile_private {
    /** The object file structure.                       */
    struct objectFile _;
    
    /** A vector with the functions of this object file. */
    struct vector_function functions;
};

struct objectFile * objectFile_new(void) {
    struct objectFile_private * self = malloc(sizeof(struct objectFile_private));
    if (self == NULL) {
        return NULL;
    }
    objectFile_create(&self->_);
    self->_.priv = self;
    vector_function_create(&self->functions);
    return &self->_;
}

void objectFile_addFunction(struct objectFile * me,
                            struct function     function) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;
    
    vector_function_push_back(&self->functions, function);
}

struct optional_function objectFile_findFunction(struct objectFile * me, uint64_t address) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;
    struct optional_function toReturn = { .has_value = false };
    
    size_t i;
    for (i = 0; i < self->functions.count && (address < self->functions.content[i].startAddress || address > self->functions.content[i].endAddress); ++i);
    
    if (i < self->functions.count) {
        toReturn.has_value = true;
        toReturn.value     = self->functions.content[i];
    }
    
    return toReturn;
}

void objectFile_functionsForEach(struct objectFile * me, void (*func)(struct function *, va_list), ...) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;
    
    va_list list;
    va_start(list, func);
    for (size_t i = 0; i < self->functions.count; ++i) {
        va_list copy;
        va_copy(copy, list);
        func(&self->functions.content[i], copy);
        va_end(copy);
    }
    va_end(list);
}

/**
 * Calls the destroy function for the given function object.
 *
 * @param f the function to be destroyed
 * @param args ignored
 */
static inline void objectFile_functionDestroy(struct function * f, va_list args) {
    (void) args;
    
    function_destroy(f);
}

void objectFile_destroy(struct objectFile * me) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;

    objectFile_functionsForEach(me, &objectFile_functionDestroy);
    vector_function_destroy(&self->functions);
    free(me->sourceFile);
    free(me->directory);
    free(me->name);
}

void objectFile_delete(struct objectFile * self) {
    objectFile_destroy(self);
    free(self->priv);
}
