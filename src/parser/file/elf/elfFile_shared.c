/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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
#include <string.h>

#include "elfFile.h"

#include "../../callstack_parser.h"
#include "../../lcs_stdio.h"

void elfFile_create(struct elfFile* self) {
    binaryFile_create(&self->_);
    
    self->_.type     = ELF_FILE;
    self->_.concrete = self;
    
    self->_.addr2String = &elfFile_addr2String;
    self->_.destroy     = &elfFile_destroy;
    self->_.deleter     = &elfFile_delete;
}

static inline char* elfFile_loadSectionStrtab64(Elf64_Ehdr* header) {
    const uint16_t index = header->e_shstrndx;
    if (index == SHN_UNDEF) {
        return NULL;
    }
    Elf64_Shdr* sect = (void*) header + header->e_shoff + index * header->e_shentsize;
    return (void*) header + sect->sh_offset;
}

static inline bool elfFile_parseSymtab64(struct elfFile* self, Elf64_Shdr* symtab, char* strBegin, void* begin) {
    void* symtabBegin = begin + symtab->sh_offset;
    uint64_t count = symtab->sh_size / sizeof(Elf64_Sym);
    Elf64_Sym* entry = symtabBegin;
    for (uint64_t i = 0; i < count; ++i, ++entry) {
        if (ELF64_ST_TYPE(entry->st_info) == STT_FUNC && entry->st_value != 0) {
            struct function f = {
                .startAddress = entry->st_value,
                .linkedName   = strdup(strBegin + entry->st_name),
                .length       = entry->st_size
            };
            elfFile_addFunction(self, f);
        }
    }
    return true;
}

static inline bool elfFile_parseFile64(struct elfFile* self, Elf64_Ehdr* buffer, dwarf_line_callback cb, va_list args) {
    if (buffer->e_shoff == 0) return false;

    char* sectStrBegin = elfFile_loadSectionStrtab64(buffer);
    if (sectStrBegin == NULL) return false;

    void* sectBegin = (void*) buffer + buffer->e_shoff;
    // TODO: e_shnum special case
    Elf64_Shdr* strtab = NULL,
              * symtab = NULL,
              * lines  = NULL;
    for (uint16_t i = 0; i < buffer->e_shnum; ++i) {
        Elf64_Shdr* current = sectBegin + i * buffer->e_shentsize;
        if (strcmp(".debug_line", sectStrBegin + current->sh_name) == 0) {
            lines = current;
            continue;
        }
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

    bool success = true;
    if (lines != NULL) {
        success = dwarf_parseLineProgram((void*) buffer + lines->sh_offset, cb, args, lines->sh_size);
    }
    return success && elfFile_parseSymtab64(self, symtab, (void*) buffer + strtab->sh_offset, buffer);
}

static inline bool elfFile_parseFile32(struct elfFile* self, Elf32_Ehdr* buffer, dwarf_line_callback cb, va_list args) {
    // TODO: Implement
    return false;
}

bool elfFile_parseFile(struct elfFile* self, void* buffer, dwarf_line_callback cb, ...) {
    va_list args;
    va_start(args, cb);
    // TODO: Care about EI_DATA

    bool success = false;
    unsigned char* e_ident = buffer;
    switch (e_ident[EI_CLASS]) {
        case ELFCLASS32: success = elfFile_parseFile32(self, buffer, cb, args); break;
        case ELFCLASS64: success = elfFile_parseFile64(self, buffer, cb, args); break;
    }

    va_end(args);
    return success;
}

bool elfFile_addr2String(struct binaryFile* me, void* address, struct callstack_frame* frame) {
    struct elfFile* self = elfFileOrNull(me);
    if (self == NULL) return false;
    
    if (!me->parsed &&
        !(me->parsed = elfFile_loadFile(self))) {
        return false;
    }

    optional_debugInfo_t result = elfFile_getDebugInfo(self, address);
    if (result.has_value) {
        if (result.value.function.linkedName == NULL) {
            return false;
        }
        char* name = (char*) result.value.function.linkedName;
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
            asprintf(&toReturn, "%s + %td", name, (ptrdiff_t) (address - self->_.startAddress - result.value.function.startAddress));
            free(name);
            frame->function = toReturn;
        }
        return true;
    }
    return false;
}

