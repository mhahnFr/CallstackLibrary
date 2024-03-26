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

#include <stddef.h>
#include <stdlib.h>

#include "elfFile.h"

#include "../debugInfo.h"

#include "../../callstack_parser.h"
#include "../../lcs_stdio.h"

struct elfFile * elfFile_new(void) {
    struct elfFile * toReturn = malloc(sizeof(struct elfFile));
    
    if (toReturn != NULL) {
        elfFile_create(toReturn);
    }
    return toReturn;
}

void elfFile_create(struct elfFile * self) {
    binaryFile_create(&self->_);
    
    self->_.type     = ELF_FILE;
    self->_.concrete = self;
    
    self->_.addr2String = &elfFile_addr2String;
    self->_.destroy     = &elfFile_destroy;
    self->_.deleter     = &elfFile_delete;
}

static inline bool elfFile_parseFile(struct elfFile* self) {
    return false;
}

static inline optional_debugInfo_t elfFile_getDebugInfo(struct elfFile* self, void* address) {
    return (optional_debugInfo_t) { .has_value = false };
}

bool elfFile_addr2String(struct binaryFile* me, void* address, struct callstack_frame* frame) {
    struct elfFile* self = elfFileOrNull(me);
    if (self == NULL) return false;
    
    if (!me->parsed &&
        !(me->parsed = elfFile_parseFile(self))) {
        return false;
    }

    optional_debugInfo_t result = elfFile_getDebugInfo(self, address);
    if (result.has_value) {
        if (result.value.function.linkedName == NULL) {
            return false;
        }
        char* name = (char*) result.value.function.linkedName;
        if (*name == '_' || *name == '\1') {
            ++name;
        }
        name = callstack_parser_demangle(name);
        if (result.value.sourceFileInfo.has_value) {
            frame->sourceFile = binaryFile_toAbsolutePath((char*) result.value.sourceFileInfo.value.sourceFile);
            frame->sourceFileRelative = binaryFile_toRelativePath((char*) result.value.sourceFileInfo.value.sourceFile);
            frame->sourceLine = result.value.sourceFileInfo.value.line;
            if (result.value.sourceFileInfo.value.column > 0) {
                frame->sourceLineColumn = (optional_ulong_t) { true, result.value.sourceFileInfo.value.column };
            }
            frame->function = name;
        } else {
            char* toReturn = NULL;
            asprintf(&toReturn, "%s + %td", name, (ptrdiff_t) -1); // TODO: Translate address
            free(name);
            frame->function = toReturn;
        }
        return true;
    }
    return false;
}

void elfFile_destroy(struct binaryFile * self) {
    (void) self;
}

void elfFile_delete(struct binaryFile * self) {
    self->destroy(self);
    free(self->concrete);
}
