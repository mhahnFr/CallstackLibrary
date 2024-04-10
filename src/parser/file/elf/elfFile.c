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

#include <elf.h>
#include <stddef.h>
#include <stdlib.h>

#include "elfFile.h"

#include "../debugInfo.h"
#include "../loader.h"

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

    vector_function_create(&self->functions);
}

static inline bool elfFile_parseFile32(struct elfFile* self, Elf32_Ehdr* buffer) {
    // TODO: Implement
    return false;
}

static inline bool elfFile_parseSymtab64(struct elfFile* self, Elf64_Shdr* symtab, char* strBegin, void* begin) {
    void* symtabBegin = begin + symtab->sh_offset;
    uint64_t count = symtab->sh_size / sizeof(Elf64_Sym);
    Elf64_Sym* entry = symtabBegin;
    for (uint64_t i = 0; i < count; ++i, ++entry) {
        if (ELF64_ST_TYPE(entry->st_info) == STT_FUNC) {
            struct function f = {
                .startAddress = entry->st_value,
                .linkedName   = strdup(strBegin + entry->st_name),
                .length       = entry->st_size
            };
            vector_function_push_back(&self->functions, f);
        }
    }
    return false;
}

static inline bool elfFile_parseFile64(struct elfFile* self, Elf64_Ehdr* buffer) {
    if (buffer->e_shoff == 0) return false;

    void* sectBegin = (void*) buffer + buffer->e_shoff;
    // TODO: e_shnum special case
    Elf64_Shdr* strtab = NULL,
              * symtab = NULL;
    for (uint16_t i = 0; i < buffer->e_shnum; ++i) {
        Elf64_Shdr* current = sectBegin + i * buffer->e_shentsize;
        bool success = true;
        switch (current->sh_type) {
            case SHT_SYMTAB:
                symtab = current;
                break;

            case SHT_STRTAB:
                // TODO: e_shstrndx special case
                if (i != buffer->e_shstrndx)
                    strtab = current;
                break;
        }
    }
    if (symtab == NULL || strtab == NULL) return false;

    return elfFile_parseSymtab64(self, symtab, (void*) buffer + strtab->sh_offset, buffer);
}

static inline bool elfFile_parseFileImpl(struct elfFile* self, void* buffer) {
    // TODO: Care about EI_DATA

    unsigned char* e_ident = buffer;
    switch (e_ident[EI_CLASS]) {
        case ELFCLASS32: return elfFile_parseFile32(self, buffer);
        case ELFCLASS64: return elfFile_parseFile64(self, buffer);
    }

    return false;
}

static inline bool elfFile_parseFile(struct elfFile* self) {
    return loader_loadFileAndExecute(self->_.fileName, (union loader_parserFunction) {
        .parseFunc = (loader_parser) elfFile_parseFileImpl
    }, false, self);
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

void elfFile_destroy(struct binaryFile* me) {
    struct elfFile* self = elfFileOrNull(me);
    if (self == NULL) return;

    vector_iterate(struct function, &self->functions, {
        function_destroy(element);
    });
    vector_function_destroy(&self->functions);
}

void elfFile_delete(struct binaryFile * self) {
    self->destroy(self);
    free(self->concrete);
}
