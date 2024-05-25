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
    lcs_section_create(&self->debugLine);
    lcs_section_create(&self->debugLineStr);
    lcs_section_create(&self->debugStr);
}

#define elfFile_loadSectionStrtab(bits)                                                      \
static inline char* elfFile_loadSectionStrtab##bits(Elf##bits##_Ehdr* header) {              \
    uint16_t index = header->e_shstrndx;                                                     \
    if (index == SHN_UNDEF) {                                                                \
        return NULL;                                                                         \
    }                                                                                        \
    if (index == SHN_XINDEX) {                                                               \
        if (header->e_shoff == 0) {                                                          \
            return NULL;                                                                     \
        }                                                                                    \
        index = ((Elf##bits##_Shdr*) ((void*) header + header->e_shoff))->sh_link;           \
    }                                                                                        \
    Elf##bits##_Shdr* sect = (void*) header + header->e_shoff + index * header->e_shentsize; \
    return (void*) header + sect->sh_offset;                                                 \
}

elfFile_loadSectionStrtab(32)
elfFile_loadSectionStrtab(64)

#define elfFile_parseSymtab(bits)                                                                                           \
static inline bool elfFile_parseSymtab##bits(struct elfFile* self, Elf##bits##_Shdr* symtab, char* strBegin, void* begin) { \
    void* symtabBegin = begin + symtab->sh_offset;                                                                          \
    uint64_t count = symtab->sh_size / sizeof(Elf##bits##_Sym);                                                             \
    Elf##bits##_Sym* entry = symtabBegin;                                                                                   \
    for (uint64_t i = 0; i < count; ++i, ++entry) {                                                                         \
        if (ELF##bits##_ST_TYPE(entry->st_info) == STT_FUNC && entry->st_value != 0) {                                      \
            struct function f = {                                                                                           \
                .startAddress = entry->st_value,                                                                            \
                .linkedName   = strdup(strBegin + entry->st_name),                                                          \
                .length       = entry->st_size                                                                              \
            };                                                                                                              \
            elfFile_addFunction(self, f);                                                                                   \
        }                                                                                                                   \
    }                                                                                                                       \
    return true;                                                                                                            \
}

elfFile_parseSymtab(32)
elfFile_parseSymtab(64)

#define elfFile_sectionToLCSSection(bits)                                                                     \
static inline struct lcs_section elfFile_sectionToLCSSection##bits(void* buffer, Elf##bits##_Shdr* section) { \
    return (struct lcs_section) {                                                                             \
        .content = buffer + section->sh_offset,                                                               \
        .size = section->sh_size                                                                              \
    };                                                                                                        \
}

elfFile_sectionToLCSSection(32)
elfFile_sectionToLCSSection(64)

#define elfFile_loadShnum(bits) \
static inline uint64_t elfFile_loadShnum##bits(Elf##bits##_Ehdr* buffer) {         \
    uint64_t shnum = buffer->e_shnum;                                              \
                                                                                   \
    if (shnum == 0) {                                                              \
        if (buffer->e_shoff == 0) {                                                \
            return shnum;                                                          \
        }                                                                          \
        shnum = ((Elf##bits##_Shdr*) ((void*) buffer + buffer->e_shoff))->sh_size; \
    }                                                                              \
    return shnum;                                                                  \
}

elfFile_loadShnum(32)
elfFile_loadShnum(64)

#define elfFile_parseFileImpl(bits)                                                             \
static inline bool elfFile_parseFile##bits (struct elfFile* self, Elf##bits##_Ehdr* buffer) {   \
    if (buffer->e_shoff == 0) return false;                                                     \
                                                                                                \
    char* sectStrBegin = elfFile_loadSectionStrtab##bits(buffer);                               \
    if (sectStrBegin == NULL) return false;                                                     \
                                                                                                \
    void* sectBegin = (void*) buffer + buffer->e_shoff;                                         \
    Elf##bits##_Shdr* strtab   = NULL,                                                          \
                    * symtab   = NULL,                                                          \
                    * dystrtab = NULL,                                                          \
                    * dysymtab = NULL;                                                          \
    uint16_t shnum = elfFile_loadShnum##bits(buffer);                                           \
    for (uint16_t i = 0; i < shnum; ++i) {                                                      \
        Elf##bits##_Shdr* current = sectBegin + i * buffer->e_shentsize;                        \
        if (strcmp(".debug_line", sectStrBegin + current->sh_name) == 0) {                      \
            self->debugLine = elfFile_sectionToLCSSection##bits(buffer, current);               \
            continue;                                                                           \
        } else if (strcmp(".debug_str", sectStrBegin + current->sh_name) == 0) {                \
            self->debugStr = elfFile_sectionToLCSSection##bits(buffer, current);                \
            continue;                                                                           \
        } else if (strcmp(".debug_line_str", sectStrBegin + current->sh_name) == 0) {           \
            self->debugLineStr = elfFile_sectionToLCSSection##bits(buffer, current);            \
            continue;                                                                           \
        }                                                                                       \
        switch (current->sh_type) {                                                             \
            case SHT_SYMTAB:                                                                    \
                symtab = current;                                                               \
                break;                                                                          \
                                                                                                \
            case SHT_DYNSYM:                                                                    \
                dysymtab = current;                                                             \
                break;                                                                          \
                                                                                                \
            case SHT_STRTAB:                                                                    \
                if (strcmp(".strtab", sectStrBegin + current->sh_name) == 0) {                  \
                    strtab = current;                                                           \
                } else if (strcmp(".dynstr", sectStrBegin + current->sh_name) == 0) {           \
                    dystrtab = current;                                                         \
                }                                                                               \
                break;                                                                          \
        }                                                                                       \
    }                                                                                           \
    if (symtab == NULL || strtab == NULL) {                                                     \
        if (dystrtab == NULL || dysymtab == NULL) {                                             \
            return false;                                                                       \
        }                                                                                       \
        symtab = dysymtab;                                                                      \
        strtab = dystrtab;                                                                      \
    }                                                                                           \
                                                                                                \
    return elfFile_parseSymtab##bits(self, symtab, (void*) buffer + strtab->sh_offset, buffer); \
}

elfFile_parseFileImpl(32)
elfFile_parseFileImpl(64)

bool elfFile_parseFile(struct elfFile* self, void* buffer, dwarf_line_callback cb, void* args) {
    // TODO: Care about EI_DATA

    bool success = false;
    unsigned char* e_ident = buffer;
    switch (e_ident[EI_CLASS]) {
        case ELFCLASS32: success = elfFile_parseFile32(self, buffer); break;
        case ELFCLASS64: success = elfFile_parseFile64(self, buffer); break;
    }

    if (success && self->debugLine.size > 0) {
        dwarf_parseLineProgram(self->debugLine, self->debugLineStr, self->debugStr, cb, args);
    }

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
            frame->sourceFileOutdated = result.value.sourceFileInfo.value.outdated;
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

bool elfFile_isOutdated(struct dwarf_sourceFile file) {
    return true;
}
