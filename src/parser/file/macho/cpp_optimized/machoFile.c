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

struct machoFile_private {
    struct machoFile _;
    
    struct objectFile* objectFiles;
    struct vector_function functions;
};

struct machoFile* machoFile_new(const char* fileName)  {
    struct machoFile_private* toReturn = malloc(sizeof(struct machoFile_private));
    
    if (toReturn != NULL) {
        machoFile_create(&toReturn->_, fileName);
        vector_function_create(&toReturn->functions);
        toReturn->objectFiles = NULL;
        toReturn->_.priv = toReturn;
    }
    return &toReturn->_;
}

void machoFile_addFunction(struct machoFile* me, struct function function) {
    struct machoFile_private* self = me->priv;
    
    vector_function_push_back(&self->functions, function);
}

void machoFile_addObjectFile(struct machoFile* me, struct objectFile* file) {
    struct machoFile_private* self = me->priv;
    
    file->next        = self->objectFiles;
    self->objectFiles = file;
}

static inline optional_debugInfo_t machoFile_createLocalDebugInfo(struct machoFile_private* self, uint64_t address) {
    struct function* closest = NULL;
    for (size_t i = 0; i < self->functions.count; ++i) {
        struct function* elem = &self->functions.content[i];

        if (closest == NULL && elem->startAddress <= address) {
            closest = elem;
        } else if (closest != NULL && elem->startAddress <= address && address - elem->startAddress < address - closest->startAddress) {
            closest = elem;
        }
    }
    if (closest == NULL) {
        return (optional_debugInfo_t) { .has_value = false };
    }
    return (optional_debugInfo_t) {
        true, (struct debugInfo) {
            .function                 = *closest,
            .sourceFileInfo.has_value = false
        }
    };
}

optional_debugInfo_t machoFile_getDebugInfo(struct machoFile* me, void* address) {
    struct machoFile_private* self = me->priv;
    
    const uint64_t searchAddress = (uint64_t) (address - self->_._.startAddress) 
                                 + (me->inMemory ? me->text_vmaddr : self->_.addressOffset);
    for (struct objectFile* it = self->objectFiles; it != NULL; it = it->next) {
        optional_debugInfo_t result = objectFile_getDebugInfoFor(it, searchAddress);
        if (result.has_value) {
            return result;
        }
    }
    
    return machoFile_createLocalDebugInfo(self, searchAddress);
}

void machoFile_destroy(struct binaryFile * me) {
    struct machoFile* tmp = machoFileOrNull(me);
    if (tmp == NULL) {
        return;
    }
    struct machoFile_private* self = tmp->priv;
    
    for (struct objectFile * tmp = self->objectFiles; tmp != NULL;) {
        struct objectFile * n = tmp->next;
        objectFile_delete(tmp);
        tmp = n;
    }
    
    for (size_t i = 0; i < self->functions.count; ++i) {
        function_destroy(&self->functions.content[i]);
    }
    vector_function_destroy(&self->functions);
}

void machoFile_delete(struct binaryFile * self) {
    self->destroy(self);
    struct machoFile* me = machoFileOrNull(self);
    if (me == NULL) {
        return;
    }
    free(me->priv);
}
