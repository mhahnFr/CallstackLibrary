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

struct machoFile * machoFile_new(void)  {
    struct machoFile * toReturn = malloc(sizeof(struct machoFile));
    
    if (toReturn != NULL) {
        machoFile_create(toReturn);
    }
    return toReturn;
}

void machoFile_addFunction(struct machoFile* self, struct function function) {
    vector_function_push_back(&self->functions, function);
}

void machoFile_addObjectFile(struct machoFile *  self,
                             struct objectFile * file) {
    file->next        = self->objectFiles;
    self->objectFiles = file;
}

struct optional_funcFile machoFile_findFunction(struct machoFile* self, void* address) {
    struct optional_funcFile toReturn = { .has_value = false };
    
    for (struct objectFile* it = self->objectFiles; it != NULL; it = it->next) {
        struct optional_function result = objectFile_findFunction(it, (uint64_t) (address - self->_.startAddress) + self->addressOffset);
        if (result.has_value) {
            toReturn = (optional_funcFile_t) {
                true, (struct pair_funcFile) {
                    result.value,
                    it
                }
            };
            break;
        }
    }
    
    return toReturn;
}

static inline optional_debugInfo_t machoFile_createLocalDebugInfo(struct machoFile* self, void* address) {
    const uint64_t searchAddress = address - self->_.startAddress + self->addressOffset;
    
    struct function* closest = NULL;
    for (size_t i = 0; i < self->functions.count; ++i) {
        struct function* elem = &self->functions.content[i];

        if (closest == NULL && elem->startAddress < searchAddress) {
            closest = elem;
        } else if (closest != NULL && elem->startAddress < searchAddress && searchAddress - elem->startAddress < searchAddress - closest->startAddress) {
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

optional_debugInfo_t machoFile_getDebugInfo(struct machoFile* self, void* address) {
    for (struct objectFile* it = self->objectFiles; it != NULL; it = it->next) {
        optional_debugInfo_t result = objectFile_getDebugInfo(it, (uint64_t) (address - self->_.startAddress) + self->addressOffset);
        if (result.has_value) {
            return result;
        }
    }
    
    return machoFile_createLocalDebugInfo(self, address);
}

void machoFile_destroy(struct binaryFile * me) {
    struct machoFile * self = machoFileOrNull(me);
    if (self == NULL) {
        return;
    }
    
    for (struct objectFile * tmp = self->objectFiles; tmp != NULL;) {
        struct objectFile * n = tmp->next;
        objectFile_delete(tmp);
        tmp = n;
    }
    
    vector_uint64_t_destroy(&self->functionStarts);
    for (size_t i = 0; i < self->functions.count; ++i) {
        function_destroy(&self->functions.content[i]);
    }
    vector_function_destroy(&self->functions);
}

void machoFile_delete(struct binaryFile * self) {
    self->destroy(self);
    free(self->concrete);
}
