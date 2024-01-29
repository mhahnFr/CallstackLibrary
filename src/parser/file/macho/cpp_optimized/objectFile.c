/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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

#include "../../dwarf/vector_dwarf_lineInfo.h"

/**
 * This structure acts as a wrapper around the object file structure.
 */
struct objectFile_private {
    /** The object file structure.                       */
    struct objectFile _;
    
    /** A vector with the functions of this object file. */
    struct vector_function functions;
    struct vector_function ownFunctions;
    struct vector_dwarfLineInfo lineInfos;
    const char* mainSourceFileCache;
};

struct objectFile * objectFile_new(void) {
    struct objectFile_private * self = malloc(sizeof(struct objectFile_private));
    if (self == NULL) {
        return NULL;
    }
    objectFile_create(&self->_);
    self->_.priv = self;
    vector_function_create(&self->functions);
    vector_function_create(&self->ownFunctions);
    vector_dwarfLineInfo_create(&self->lineInfos);
    self->mainSourceFileCache = NULL;
    return &self->_;
}

void objectFile_addFunction(struct objectFile * me,
                            struct function     function) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;
    
    vector_function_push_back(&self->functions, function);
}

void objectFile_addOwnFunction(struct objectFile* me,
                               struct function    function) {
    struct objectFile_private* self = me->priv;
    
    vector_function_push_back(&self->ownFunctions, function);
}

static inline void objectFile_dwarfLineCallback(struct dwarf_lineInfo info, va_list args) {
    struct objectFile_private* self = va_arg(args, void*);
    
    vector_dwarfLineInfo_push_back(&self->lineInfos, info);
}

static inline bool objectFile_parseIntern(struct objectFile_private* self) {
    const bool result = objectFile_parse(&self->_, objectFile_dwarfLineCallback, self);
    if (!result) {
        for (size_t i = 0; i < self->ownFunctions.count; ++i) {
            function_destroy(&self->ownFunctions.content[i]);
        }
        vector_function_clear(&self->ownFunctions);
    }
    return result;
}

static inline optional_function_t objectFile_findOwnFunction(struct objectFile_private* self, const char* name) {
    optional_function_t toReturn = { .has_value = false };
    
    for (size_t i = 0; i < self->ownFunctions.count; ++i) {
        if (strcmp(name, self->ownFunctions.content[i].linkedName) == 0) {
            toReturn = (struct optional_function) { true, self->ownFunctions.content[i] };
            break;
        }
    }
    
    return toReturn;
}

static inline const char* objectFile_getSourceFileName(struct objectFile_private* self) {
    if (self->mainSourceFileCache != NULL) return self->mainSourceFileCache;
    
    const size_t size = strlen(self->_.directory) + strlen(self->_.sourceFile) + 1;
    char* toReturn = malloc(size);
    if (toReturn == NULL) {
        return self->_.sourceFile;
    }
    strlcpy(toReturn, self->_.directory, size);
    strlcat(toReturn, self->_.sourceFile, size);
    return self->mainSourceFileCache = toReturn;
}

optional_debugInfo_t objectFile_getDebugInfo(struct objectFile* me, uint64_t address, struct function function) {
    optional_debugInfo_t toReturn = { .has_value = false };
    
    toReturn = (optional_debugInfo_t) {
        true, (struct debugInfo) {
            .function = function,
            .sourceFileInfo.has_value = false
        }
    };
    struct objectFile_private* self = (struct objectFile_private*) me->priv;
    
    if (!me->parsed) {
        if (!(me->parsed = objectFile_parseIntern(self))) {
            return toReturn;
        }
    }
    optional_function_t ownFunction = objectFile_findOwnFunction(self, function.linkedName);
    if (!ownFunction.has_value) {
        return toReturn;
    }
    const uint64_t lineAddress = ownFunction.value.startAddress + address - function.startAddress;
    
    struct dwarf_lineInfo* closest = NULL;
    for (size_t i = 0; i < self->lineInfos.count; ++i) {
        struct dwarf_lineInfo* elem = &self->lineInfos.content[i];
        
        if (closest == NULL && elem->address < lineAddress) {
            closest = elem;
        } else if (closest != NULL && elem->address < lineAddress && lineAddress - elem->address < lineAddress - closest->address) {
            closest = elem;
        }
    }
    if (closest == NULL) {
        return toReturn;
    }
    toReturn.value.sourceFileInfo = (optional_sourceFileInfo_t) {
        true, (struct sourceFileInfo) {
            closest->line,
            closest->column,
            closest->fileName == NULL ? objectFile_getSourceFileName(self) : closest->fileName
        }
    };
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
    for (size_t i = 0; i < self->ownFunctions.count; ++i) {
        function_destroy(&self->ownFunctions.content[i]);
    }
    vector_function_destroy(&self->ownFunctions);
    for (size_t i = 0; i < self->lineInfos.count; ++i) {
        dwarf_lineInfo_destroy(&self->lineInfos.content[i]);
    }
    vector_dwarfLineInfo_destroy(&self->lineInfos);
    free((void*) self->mainSourceFileCache);
    free(me->sourceFile);
    free(me->directory);
    free(me->name);
}

void objectFile_delete(struct objectFile * self) {
    objectFile_destroy(self);
    free(self->priv);
}
