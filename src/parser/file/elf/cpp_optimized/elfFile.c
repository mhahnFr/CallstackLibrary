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

#include <stdlib.h>

#include "../elfFile.h"

#include "../../loader.h"
#include "../../vector_function.h"

#include "../../dwarf/vector_dwarf_lineInfo.h"

struct elfFile_private {
    struct elfFile _;

    vector_function_t functions;
    vector_dwarfLineInfo_t lineInfos;
};

struct elfFile* elfFile_new(void) {
    struct elfFile_private* toReturn = malloc(sizeof(struct elfFile_private));
    
    if (toReturn != NULL) {
        elfFile_create(&toReturn->_);
        toReturn->_.priv = toReturn;
        vector_dwarfLineInfo_create(&toReturn->lineInfos);
        vector_function_create(&toReturn->functions);
    }
    return &toReturn->_;
}

void elfFile_addFunction(struct elfFile* me, struct function f) {
    struct elfFile_private* self = me->priv;

    vector_function_push_back(&self->functions, f);
}

static inline void elfFile_lineProgramCallback(struct dwarf_lineInfo info, void* args) {
    struct elfFile_private* self = args;

    vector_dwarfLineInfo_push_back(&self->lineInfos, info);
}

static inline bool elfFile_loadFileImpl(struct elfFile_private* self, void* buffer) {
    return elfFile_parseFile(&self->_, buffer, elfFile_lineProgramCallback, self);
}

bool elfFile_loadFile(struct elfFile* me) {
    struct elfFile_private* self = me->priv;

    return loader_loadFileAndExecute(me->_.fileName, (union loader_parserFunction) {
        .parseFunc = (loader_parser) elfFile_loadFileImpl
    }, false, self);
}

optional_debugInfo_t elfFile_getDebugInfo(struct elfFile* me, void* address) {
    struct elfFile_private* self = me->priv;

    optional_debugInfo_t toReturn = { .has_value = false };

    const uint64_t translated = (uintptr_t) address - (uintptr_t) me->_.startAddress;
    struct function* closest = NULL;
    vector_iterate(struct function, &self->functions, {
        if (closest == NULL && element->startAddress <= translated) {
            closest = element;
        } else if (closest != NULL && element->startAddress <= translated && translated - element->startAddress < translated - closest->startAddress) {
            closest = element;
        }
    })
    
    if (closest == NULL
        || closest->startAddress > translated
        || closest->startAddress + closest->length < translated) {
        return toReturn;
    }
    toReturn = (optional_debugInfo_t) {
        .has_value = true,
        .value = (struct debugInfo) {
            .function = *closest,
            .sourceFileInfo = { .has_value = false }
        }
    };
    
    struct dwarf_lineInfo* closestInfo = NULL;
    vector_iterate(struct dwarf_lineInfo, &self->lineInfos, {
        if (closestInfo == NULL && element->address < translated) {
            closestInfo = element;
        } else if (closestInfo != NULL && element->address < translated && translated - element->address < translated - closestInfo->address) {
            closestInfo = element;
        }
    })
    if (closestInfo == NULL
        || closest->startAddress >= closestInfo->address
        || closest->startAddress + closest->length < closestInfo->address) {
        return toReturn;
    }
    toReturn.value.sourceFileInfo = (optional_sourceFileInfo_t) {
        .has_value = true,
        .value = {
            closestInfo->line,
            closestInfo->column,
            closestInfo->fileName
        }
    };
    return toReturn;
}

void elfFile_destroy(struct binaryFile* me) {
    struct elfFile* tmp = elfFileOrNull(me);
    if (tmp == NULL) return;

    struct elfFile_private* self = tmp->priv;

    vector_iterate(struct function, &self->functions, function_destroy(element);)
    vector_function_destroy(&self->functions);
    vector_dwarfLineInfo_destroyWith(&self->lineInfos, dwarf_lineInfo_destroyValue);
}

void elfFile_delete(struct binaryFile* self) {
    self->destroy(self);
    struct elfFile* me = elfFileOrNull(self);
    free(me->priv);
}
