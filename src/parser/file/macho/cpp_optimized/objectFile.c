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
    /** The object file structure.                             */
    struct objectFile _;
    
    /** The functions found in the represented object file.    */
    struct vector_function ownFunctions;
    /** The DWARF line infos found in the object file.         */
    struct vector_dwarfLineInfo lineInfos;
    /** Cache for the full name of the referenced source file. */
    const char* mainSourceFileCache;
};

struct objectFile * objectFile_new(void) {
    struct objectFile_private * self = malloc(sizeof(struct objectFile_private));
    if (self == NULL) {
        return NULL;
    }
    objectFile_create(&self->_);
    self->_.priv = self;
    vector_function_create(&self->ownFunctions);
    vector_dwarfLineInfo_create(&self->lineInfos);
    self->mainSourceFileCache = NULL;
    return &self->_;
}

void objectFile_addOwnFunction(struct objectFile* me,
                               struct function    function) {
    struct objectFile_private* self = me->priv;
    
    vector_function_push_back(&self->ownFunctions, function);
}

/**
 * The callback function for adding a deducted DWARF line information.
 *
 * @param info the DWARF line info entry
 * @param args the variadic arguments - should include as the first argument the object file object
 */
static inline void objectFile_dwarfLineCallback(struct dwarf_lineInfo info, va_list args) {
    struct objectFile_private* self = va_arg(args, void*);
    
    vector_dwarfLineInfo_push_back(&self->lineInfos, info);
}

/**
 * @brief Parses the represented object file.
 *
 * If the parsing fails, the intermediate objects are cleaned up.
 *
 * @param self the object file object
 * @return whether the parsing was successful
 */
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

/**
 * Finds and returns the function with the given name deducted from the represented object file.
 *
 * @param self the object file object
 * @param name the name of the desired function
 * @return the optionally found function
 */
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

/**
 * @brief Returns the full source file name found in the referencing Mach-O executable / library file.
 *
 * @note Do not free the returned string.
 *
 * @param self the object file object
 * @return the full source file name or `NULL` if the allocation failed
 */
static inline const char* objectFile_getSourceFileName(struct objectFile_private* self) {
    if (self->mainSourceFileCache != NULL) return self->mainSourceFileCache;
    if (self->_.directory == NULL || self->_.sourceFile == NULL) return "<< Unknown >>";
    
    const size_t size = strlen(self->_.directory) + strlen(self->_.sourceFile) + 1;
    char* toReturn = malloc(size);
    if (toReturn == NULL) {
        return self->_.sourceFile;
    }
    strlcpy(toReturn, self->_.directory, size);
    strlcat(toReturn, self->_.sourceFile, size);
    toReturn[size - 1] = '\0';
    return self->mainSourceFileCache = toReturn;
}

bool objectFile_parseBuffer(struct objectFile* me, void* buffer) {
    struct objectFile_private* self = me->priv;
    
    const bool result = objectFile_parseWithBuffer(me, buffer, objectFile_dwarfLineCallback, self);
    if (!result) {
        for (size_t i = 0; i < self->ownFunctions.count; ++i) {
            function_destroy(&self->ownFunctions.content[i]);
        }
        vector_function_clear(&self->ownFunctions);
    }
    return result;
}

optional_debugInfo_t objectFile_getDebugInfo(struct objectFile* me, uint64_t address, struct function function) {
    optional_debugInfo_t       toReturn = { .has_value = false };
    struct objectFile_private* self     = me->priv;
    
    if (!me->parsed) {
        if (!(me->parsed = objectFile_parseIntern(self))) {
            return toReturn;
        }
    }
    uint64_t lineAddress;
    if (me->isDsymBundle) {
        lineAddress = address;
    } else {
        optional_function_t ownFunction = objectFile_findOwnFunction(self, function.linkedName);
        if (!ownFunction.has_value) {
            return toReturn;
        }
        lineAddress = ownFunction.value.startAddress + address - function.startAddress;
    }
    
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
    return (optional_debugInfo_t) {
        true, (struct debugInfo) {
            function, (optional_sourceFileInfo_t) {
                true, (struct sourceFileInfo) {
                    closest->line,
                    closest->column,
                    closest->fileName == NULL ? objectFile_getSourceFileName(self) : closest->fileName
                }
            }
        }
    };
}

uint8_t* objectFile_getUUID(struct objectFile* me) {
    if (!me->parsed) {
        me->parsed = objectFile_parseIntern(me->priv);
    }
    return me->uuid;
}

void objectFile_destroy(struct objectFile * me) {
    struct objectFile_private * self = (struct objectFile_private *) me->priv;

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
