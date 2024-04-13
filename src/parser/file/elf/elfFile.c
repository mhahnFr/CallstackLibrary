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

#include "../dwarf/dwarf_parser.h"

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
    vector_dwarfLineInfo_create(&self->lineInfos);
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
        if (ELF64_ST_TYPE(entry->st_info) == STT_FUNC && entry->st_value != 0) {
            struct function f = {
                .startAddress = entry->st_value,
                .linkedName   = strdup(strBegin + entry->st_name),
                .length       = entry->st_size
            };
            vector_function_push_back(&self->functions, f);
        }
    }
    return true;
}

static inline char* elfFile_loadSectionStrtab(Elf64_Ehdr* header) {
    const uint16_t index = header->e_shstrndx;
    if (index == SHN_UNDEF) {
        return NULL;
    }
    Elf64_Shdr* sect = (void*) header + header->e_shoff + index * header->e_shentsize;
    return (void*) header + sect->sh_offset;
}

static inline void elfFile_lineProgramCallback(struct dwarf_lineInfo info, va_list args) {
    struct elfFile* self = va_arg(args, void*);

    vector_dwarfLineInfo_push_back(&self->lineInfos, info);
}

static inline bool elfFile_parseLineProgram(void* section, uint64_t size, ...) {
    va_list args;
    va_start(args, size);
    const bool success = dwarf_parseLineProgram(section, elfFile_lineProgramCallback, args, size);
    va_end(args);
    return success;
}

static inline bool elfFile_parseFile64(struct elfFile* self, Elf64_Ehdr* buffer) {
    if (buffer->e_shoff == 0) return false;

    char* sectStrBegin = elfFile_loadSectionStrtab(buffer);
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
        success = elfFile_parseLineProgram((void*) buffer + lines->sh_offset, lines->sh_size, self);
    }
    return success && elfFile_parseSymtab64(self, symtab, (void*) buffer + strtab->sh_offset, buffer);
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
    optional_debugInfo_t toReturn = { .has_value = false };

    const uint64_t translated = (uint64_t) address - (uint64_t) self->_.startAddress;
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
        || closest->startAddress >= closestInfo->address // FIXME: Is the startAddress not inclusive?
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

void elfFile_destroy(struct binaryFile* me) {
    struct elfFile* self = elfFileOrNull(me);
    if (self == NULL) return;

    vector_iterate(struct function, &self->functions, {
        function_destroy(element);
    });
    vector_function_destroy(&self->functions);
    vector_dwarfLineInfo_destroyWith(&self->lineInfos, dwarf_lineInfo_destroyValue);
}

void elfFile_delete(struct binaryFile * self) {
    self->destroy(self);
    free(self->concrete);
}
