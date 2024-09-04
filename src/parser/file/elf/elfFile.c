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
#include <stdlib.h>

#include <elf/elfUtils.h>
#include <file/pathUtils.h>

#include "elfFile.h"

#include "../bounds.h"
#include "../loader.h"
#include "../dwarf/dwarf_parser.h"

#include "../../lcs_stdio.h"
#include "../../callstack_parser.h"

struct elfFile* elfFile_new(void) {
    struct elfFile* toReturn = malloc(sizeof(struct elfFile));
    
    if (toReturn != NULL) {
        elfFile_create(toReturn);
    }
    return toReturn;
}

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
    lcs_section_create(&self->debugInfo);
    lcs_section_create(&self->debugAbbrev);
    lcs_section_create(&self->debugStrOffsets);
    vector_dwarfLineInfo_create(&self->lineInfos);
    vector_function_create(&self->functions);
}

/**
 * @brief The callback for the DWARF parser.
 *
 * Stores the given DWARF line table row.
 *
 * @param info the deducted DWARF line table row
 * @param args the payload, expected to be a private elf file structure object
 */
static inline void elfFile_lineProgramCallback(struct dwarf_lineInfo info, void* args) {
    struct elfFile* self = args;

    vector_dwarfLineInfo_push_back(&self->lineInfos, info);
}

#define elfFile_loadSectionStrtab(bits)                                                                                         \
static inline char* elfFile_loadSectionStrtab##bits(Elf##bits##_Ehdr* header, bool littleEndian) {                              \
    uint16_t index = ELF_TO_HOST(16, header->e_shstrndx, littleEndian);                                                         \
    if (index == SHN_UNDEF) {                                                                                                   \
        return NULL;                                                                                                            \
    }                                                                                                                           \
    if (index == SHN_XINDEX) {                                                                                                  \
        if (ELF_TO_HOST(bits, header->e_shoff, littleEndian) == 0) {                                                            \
            return NULL;                                                                                                        \
        }                                                                                                                       \
        index = ELF_TO_HOST(32,                                                                                                 \
                            ((Elf##bits##_Shdr*) ((void*) header + ELF_TO_HOST(bits, header->e_shoff, littleEndian)))->sh_link, \
                            littleEndian);                                                                                      \
    }                                                                                                                           \
    Elf##bits##_Shdr* sect = (void*) header + ELF_TO_HOST(bits, header->e_shoff, littleEndian)                                  \
                             + index * ELF_TO_HOST(16, header->e_shentsize, littleEndian);                                      \
    return (void*) header + ELF_TO_HOST(bits, sect->sh_offset, littleEndian);                                                   \
}

elfFile_loadSectionStrtab(32)
elfFile_loadSectionStrtab(64)

#define elfFile_parseSymtab(bits)                                                                                       \
static inline bool elfFile_parseSymtab##bits(struct elfFile*   self,                                                    \
                                             Elf##bits##_Shdr* symtab,                                                  \
                                             char*             strBegin,                                                \
                                             void*             begin,                                                   \
                                             bool              littleEndian) {                                          \
    void* symtabBegin = begin + ELF_TO_HOST(bits, symtab->sh_offset, littleEndian);                                     \
    uint64_t count = ELF_TO_HOST(bits, symtab->sh_size, littleEndian) / sizeof(Elf##bits##_Sym);                        \
    Elf##bits##_Sym* entry = symtabBegin;                                                                               \
    for (uint64_t i = 0; i < count; ++i, ++entry) {                                                                     \
        if (ELF##bits##_ST_TYPE(entry->st_info) == STT_FUNC && ELF_TO_HOST(bits, entry->st_value, littleEndian) != 0) { \
            struct function f = {                                                                                       \
                .startAddress = ELF_TO_HOST(bits, entry->st_value, littleEndian),                                       \
                .linkedName   = strdup(strBegin + ELF_TO_HOST(32, entry->st_name, littleEndian)),                       \
                .length       = ELF_TO_HOST(bits, entry->st_size, littleEndian)                                         \
            };                                                                                                          \
            vector_function_push_back(&self->functions, f);                                                             \
        }                                                                                                               \
    }                                                                                                                   \
    return true;                                                                                                        \
}

elfFile_parseSymtab(32)
elfFile_parseSymtab(64)

#define elfFile_sectionToLCSSection(bits)                                                            \
static inline struct lcs_section elfFile_sectionToLCSSection##bits(void*             buffer,         \
                                                                   Elf##bits##_Shdr* section,        \
                                                                   bool              littleEndian) { \
    return (struct lcs_section) {                                                                    \
        .content = buffer + ELF_TO_HOST(bits, section->sh_offset, littleEndian),                     \
        .size = ELF_TO_HOST(bits, section->sh_size, littleEndian)                                    \
    };                                                                                               \
}

elfFile_sectionToLCSSection(32)
elfFile_sectionToLCSSection(64)

#define elfFile_loadShnum(bits) \
static inline uint64_t elfFile_loadShnum##bits(Elf##bits##_Ehdr* buffer, bool littleEndian) {                                   \
    uint64_t shnum = ELF_TO_HOST(16, buffer->e_shnum, littleEndian);                                                            \
                                                                                                                                \
    if (shnum == 0) {                                                                                                           \
        if (ELF_TO_HOST(bits, buffer->e_shoff, littleEndian) == 0) {                                                            \
            return shnum;                                                                                                       \
        }                                                                                                                       \
        shnum = ELF_TO_HOST(bits,                                                                                               \
                            ((Elf##bits##_Shdr*) ((void*) buffer + ELF_TO_HOST(bits, buffer->e_shoff, littleEndian)))->sh_size, \
                            littleEndian);                                                                                      \
    }                                                                                                                           \
    return shnum;                                                                                                               \
}

elfFile_loadShnum(32)
elfFile_loadShnum(64)

#define elfFile_parseFileImpl(bits)                                                                                  \
static inline bool elfFile_parseFile##bits (struct elfFile* self, Elf##bits##_Ehdr* buffer, bool littleEndian) {     \
    if (ELF_TO_HOST(bits, buffer->e_shoff, littleEndian) == 0) return false;                                         \
                                                                                                                     \
    char* sectStrBegin = elfFile_loadSectionStrtab##bits(buffer, littleEndian);                                      \
    if (sectStrBegin == NULL) return false;                                                                          \
                                                                                                                     \
    void* sectBegin = (void*) buffer + ELF_TO_HOST(bits, buffer->e_shoff, littleEndian);                             \
    Elf##bits##_Shdr* strtab   = NULL,                                                                               \
                    * symtab   = NULL,                                                                               \
                    * dystrtab = NULL,                                                                               \
                    * dysymtab = NULL;                                                                               \
    uint16_t shnum = elfFile_loadShnum##bits(buffer, littleEndian);                                                  \
    for (uint16_t i = 0; i < shnum; ++i) {                                                                           \
        Elf##bits##_Shdr* current = sectBegin + i * ELF_TO_HOST(16, buffer->e_shentsize, littleEndian);              \
        const char* sectionName = sectStrBegin + ELF_TO_HOST(32, current->sh_name, littleEndian);                    \
        if (strcmp(".debug_line", sectionName) == 0) {                                                               \
            self->debugLine = elfFile_sectionToLCSSection##bits(buffer, current, littleEndian);                      \
            continue;                                                                                                \
        } else if (strcmp(".debug_str", sectionName) == 0) {                                                         \
            self->debugStr = elfFile_sectionToLCSSection##bits(buffer, current, littleEndian);                       \
            continue;                                                                                                \
        } else if (strncmp(".debug_line_str", sectionName, 16) == 0) {                                               \
            self->debugLineStr = elfFile_sectionToLCSSection##bits(buffer, current, littleEndian);                   \
            continue;                                                                                                \
        } else if (strcmp(".debug_info", sectionName) == 0) {                                                        \
            self->debugInfo = elfFile_sectionToLCSSection##bits(buffer, current, littleEndian);                      \
            continue;                                                                                                \
        } else if (strcmp(".debug_abbrev", sectionName) == 0) {                                                      \
            self->debugAbbrev = elfFile_sectionToLCSSection##bits(buffer, current, littleEndian);                    \
            continue;                                                                                                \
        } else if (strcmp(".debug_str_offsets", sectionName) == 0) {                                                 \
            self->debugStrOffsets = elfFile_sectionToLCSSection##bits(buffer, current, littleEndian);                \
        }                                                                                                            \
        switch (ELF_TO_HOST(32, current->sh_type, littleEndian)) {                                                   \
            case SHT_SYMTAB:                                                                                         \
                symtab = current;                                                                                    \
                break;                                                                                               \
                                                                                                                     \
            case SHT_DYNSYM:                                                                                         \
                dysymtab = current;                                                                                  \
                break;                                                                                               \
                                                                                                                     \
            case SHT_STRTAB:                                                                                         \
                if (strcmp(".strtab", sectStrBegin + ELF_TO_HOST(32, current->sh_name, littleEndian)) == 0) {        \
                    strtab = current;                                                                                \
                } else if (strcmp(".dynstr", sectStrBegin + ELF_TO_HOST(32, current->sh_name, littleEndian)) == 0) { \
                    dystrtab = current;                                                                              \
                }                                                                                                    \
                break;                                                                                               \
        }                                                                                                            \
    }                                                                                                                \
    if (symtab == NULL || strtab == NULL) {                                                                          \
        if (dystrtab == NULL || dysymtab == NULL) {                                                                  \
            return false;                                                                                            \
        }                                                                                                            \
        symtab = dysymtab;                                                                                           \
        strtab = dystrtab;                                                                                           \
    }                                                                                                                \
                                                                                                                     \
    return elfFile_parseSymtab##bits(self,                                                                           \
                                     symtab,                                                                         \
                                     (void*) buffer + ELF_TO_HOST(bits, strtab->sh_offset, littleEndian),            \
                                     buffer,                                                                         \
                                     littleEndian);                                                                  \
}

elfFile_parseFileImpl(32)
elfFile_parseFileImpl(64)

/**
 * Parses the given ELF file into the given abstraction object.
 *
 * @param self the ELF file abstraction object
 * @param buffer the start pointer of the ELF file
 * @return whether the file was parsed successfully
 */
static inline bool elfFile_parseFile(struct elfFile* self, void* buffer) {
    bool success = false;
    unsigned char* e_ident = buffer;
    switch (e_ident[EI_CLASS]) {
        case ELFCLASS32: success = elfFile_parseFile32(self, buffer, e_ident[EI_DATA] == ELFDATA2LSB); break;
        case ELFCLASS64: success = elfFile_parseFile64(self, buffer, e_ident[EI_DATA] == ELFDATA2LSB); break;
    }

    if (success && self->debugLine.size > 0) {
        dwarf_parseLineProgram(self->debugLine,
                               self->debugLineStr,
                               self->debugStr,
                               self->debugInfo,
                               self->debugAbbrev,
                               self->debugStrOffsets,
                               elfFile_lineProgramCallback, self);
    }

    return success;
}

/**
 * @brief Returns how the two given functions compare.
 *
 * Sorted descendingly.
 *
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return `0` if the two functions compare equal or a value smaller or greater than `0` according to the sorting order
 */
static inline int elfFile_functionCompare(const void* lhs, const void* rhs) {
    const struct function* a = lhs,
                         * b = rhs;
    if (a->startAddress < b->startAddress) return +1;
    if (a->startAddress > b->startAddress) return -1;

    return 0;
}

/**
 * @brief Returns how the two given DWARF line infos compare.
 *
 * Sorted descendingly.
 *
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return `0` if the two infos compare equal or a value smaller or greater than `0` according to the sorting order
 */
static inline int elfFile_lineInfoCompare(const void* lhs, const void* rhs) {
    const struct dwarf_lineInfo* a = lhs,
                               * b = rhs;

    if (a->address < b->address) return +1;
    if (a->address > b->address) return -1;

    return 0;
}

/**
 * Loads the ELF file represented by the given abstraction object.
 *
 * @param self the ELF file abstraction object
 * @return whether the ELF file was loaded successfully
 */
static inline bool elfFile_loadFile(struct elfFile* self) {
    const bool success =  loader_loadFileAndExecute(self->_.fileName, (union loader_parserFunction) {
        .parseFunc = (loader_parser) elfFile_parseFile
    }, false, self);
    if (success) {
        qsort(self->functions.content, self->functions.count, sizeof(struct function), elfFile_functionCompare);
        qsort(self->lineInfos.content, self->lineInfos.count, sizeof(struct dwarf_lineInfo), elfFile_lineInfoCompare);
    } else {
        vector_iterate(struct function, &self->functions, function_destroy(element);)
        vector_function_clear(&self->functions);
    }
    return success;
}

/**
 * Deducts the debugging information available for the given address in the given
 * ELF file abstraction object.
 *
 * @param self the ELF file abstraction object to be searched
 * @param address the address to be translated
 * @return the optionally available debug information
 */
static inline optional_debugInfo_t elfFile_getDebugInfo(struct elfFile* self, void* address) {
    optional_debugInfo_t toReturn = { .has_value = false };

    const uint64_t translated = (uintptr_t) address - self->_.relocationOffset;
    struct function tmp = (struct function) { .startAddress = translated };
    const struct function* closest = lower_bound(&tmp,
                                                 self->functions.content,
                                                 self->functions.count,
                                                 sizeof(struct function),
                                                 elfFile_functionCompare);
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
    
    struct dwarf_lineInfo tmpInfo = (struct dwarf_lineInfo) { .address = translated };
    const struct dwarf_lineInfo* closestInfo = upper_bound(&tmpInfo,
                                                           self->lineInfos.content,
                                                           self->lineInfos.count,
                                                           sizeof(struct dwarf_lineInfo),
                                                           elfFile_lineInfoCompare);
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
            closestInfo->sourceFile.fileName,
            binaryFile_isOutdated(closestInfo->sourceFile)
        }
    };
    return toReturn;
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
            frame->sourceFile = path_toAbsolutePath((char*) result.value.sourceFileInfo.value.sourceFile);
            frame->sourceFileRelative = path_toRelativePath((char*) result.value.sourceFileInfo.value.sourceFile);
            frame->sourceFileOutdated = result.value.sourceFileInfo.value.outdated;
            frame->sourceLine = result.value.sourceFileInfo.value.line;
            frame->sourceLineColumn = result.value.sourceFileInfo.value.column;
            frame->function = name;
        } else {
            char* toReturn = NULL;
            asprintf(&toReturn, "%s + %td", name, (ptrdiff_t) (address - self->_.relocationOffset - result.value.function.startAddress));
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

    vector_iterate(struct function, &self->functions, function_destroy(element);)
    vector_function_destroy(&self->functions);
    vector_dwarfLineInfo_destroyWith(&self->lineInfos, dwarf_lineInfo_destroyValue);
}

void elfFile_delete(struct binaryFile* self) {
    self->destroy(self);
    struct elfFile* me = elfFileOrNull(self);
    free(me);
}
