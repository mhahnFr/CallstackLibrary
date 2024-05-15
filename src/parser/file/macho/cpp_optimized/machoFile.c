/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
 *
 * This file is part of the CallstackLibrary.
 *
 * The CallstackLibrary is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The CallstackLibrary is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * CallstackLibrary, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "../machoFile.h"

#include "../vector_pair_funcFile.h"

/**
 * This structure contains the additions needed by this C implementation
 * of the Mach-O file abstraction.
 */
struct machoFile_private {
    /** The super part of this structure.        */
    struct machoFile _;
    
    /** The function / object file object pairs. */
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
            function_destroy(&self->functions.content[i].first);
            self->functions.content[i] = function;
        } else {
            function_destroy(&function.first);
        }
    }
}

optional_debugInfo_t machoFile_getDebugInfo(struct machoFile* me, void* address) {
    struct machoFile_private* self = me->priv;
    
    const uint64_t searchAddress = (uintptr_t) (address - self->_._.startAddress)
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
    
    if (closest == NULL
        || (closest->first.length != 0 && closest->first.startAddress + closest->first.length < searchAddress)) {
        return (optional_debugInfo_t) { .has_value = false };
    }
    optional_debugInfo_t info = { .has_value = false };
    if (machoFile_getDSYMBundle(me) != NULL && memcmp(me->uuid, objectFile_getUUID(me->dSYMFile.file), 16) == 0) {
        info = objectFile_getDebugInfo(me->dSYMFile.file, searchAddress, closest->first);
        if (info.has_value) {
            return info;
        }
    }
    if (closest->second == NULL) {
        return (optional_debugInfo_t) {
            true, (struct debugInfo) {
                closest->first,
                .sourceFileInfo.has_value = false
            }
        };
    }
    info = objectFile_getDebugInfo(closest->second, searchAddress, closest->first);
    if (!info.has_value) {
        info = (optional_debugInfo_t) {
            true, (struct debugInfo) {
                closest->first,
                .sourceFileInfo.has_value = false
            }
        };
    }
    return info;
}

void machoFile_destroy(struct binaryFile * me) {
    struct machoFile* tmp = machoFileOrNull(me);
    if (tmp == NULL) {
        return;
    }
    struct machoFile_private* self = tmp->priv;
    
    vector_iterate(pair_funcFile_t, &self->functions, function_destroy(&element->first);)
    vector_pairFuncFile_destroy(&self->functions);
    if (tmp->dSYMFile.file != NULL) {
        objectFile_delete(tmp->dSYMFile.file);
    }
}

void machoFile_delete(struct binaryFile * self) {
    self->destroy(self);
    struct machoFile* me = machoFileOrNull(self);
    if (me == NULL) {
        return;
    }
    free(me->priv);
}
