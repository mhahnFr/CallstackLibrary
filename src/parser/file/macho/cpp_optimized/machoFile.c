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

#include "../machoFile.h"

#include "../vector_pairFuncFile.h"

struct machoFile_private {
    struct machoFile _;
    
    struct vector_pairFuncFile functions;
};

struct machoFile* machoFile_new(const char* fileName)  {
    struct machoFile_private* toReturn = malloc(sizeof(struct machoFile_private));
    
    if (toReturn != NULL) {
        machoFile_create(&toReturn->_, fileName);
        vector_pairFuncFile_create(&toReturn->functions);
        toReturn->_.priv = toReturn;
    }
    return &toReturn->_;
}

void machoFile_addFunction(struct machoFile* me, pair_funcFile_t function) {
    struct machoFile_private* self = me->priv;
    
    size_t i;
    for (i = 0; i < self->functions.count && function.first.startAddress != self->functions.content[i].first.startAddress; ++i);
    
    if (i == self->functions.count) {
        vector_pairFuncFile_push_back(&self->functions, function);
    } else {
        if (self->functions.content[i].second == NULL) {
            self->functions.content[i] = function;
        }
    }
}

optional_debugInfo_t machoFile_getDebugInfo(struct machoFile* me, void* address) {
    struct machoFile_private* self = me->priv;
    
    const uint64_t searchAddress = (uint64_t) (address - self->_._.startAddress) 
                                 + (me->inMemory ? me->text_vmaddr : self->_.addressOffset);
    
    pair_funcFile_t* closest = NULL;
    vector_iterate(pair_funcFile_t, &self->functions, {
        if (closest == NULL && element->first.startAddress <= searchAddress) {
            closest = element;
        } else if (closest != NULL
                   && element->first.startAddress <= searchAddress
                   && searchAddress - element->first.startAddress < searchAddress - closest->first.startAddress) {
            closest = element;
        }
    })
    
    if (closest == NULL) {
        return (optional_debugInfo_t) { .has_value = false };
    } else if (closest->second == NULL) {
        return (optional_debugInfo_t) {
            true, (struct debugInfo) {
                .function = closest->first,
                .sourceFileInfo.has_value = false
            }
        };
    }
    return objectFile_getDebugInfo(closest->second, searchAddress, closest->first);
}

void machoFile_destroy(struct binaryFile * me) {
    struct machoFile* tmp = machoFileOrNull(me);
    if (tmp == NULL) {
        return;
    }
    struct machoFile_private* self = tmp->priv;
    
    vector_iterate(pair_funcFile_t, &self->functions, function_destroy(&element->first);)
    vector_pairFuncFile_destroy(&self->functions);
}

void machoFile_delete(struct binaryFile * self) {
    self->destroy(self);
    struct machoFile* me = machoFileOrNull(self);
    if (me == NULL) {
        return;
    }
    free(me->priv);
}
