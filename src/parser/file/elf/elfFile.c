/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2026  mhahnFr
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

#include "elfFile.h"

#include <elf.h>
#include <stdlib.h>
#include <elf/elfUtils.h>
#include <file/pathUtils.h>
#include <misc/string_utils.h>

#include "lcs_stdio.h"
#include "../bounds.h"
#include "../debugInfo.h"
#include "../loader.h"
#include "../../callstack_parser.h"
#include "../dwarf/dwarf_parser.h"

void elfFile_create(struct elfFile* self) {
    lcs_section_create(&self->debugLine);
    lcs_section_create(&self->debugLineStr);
    lcs_section_create(&self->debugStr);
    lcs_section_create(&self->debugInfo);
    lcs_section_create(&self->debugAbbrev);
    lcs_section_create(&self->debugStrOffsets);
    vector_init(&self->lineInfos);
    vector_init(&self->functions);
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

    vector_push_back(&self->lineInfos, info);
}

/**
 * Generates an implementation for parsing the string table section.
 *
 * @param bits the amount of bits the implementation shall handle
 */
#define elfFile_loadSectionStrtab(bits)                                                                                         \
static inline char* elfFile_loadSectionStrtab##bits(const Elf##bits##_Ehdr* header, bool littleEndian) {                        \
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

/**
 * Generates an implementation for parsing the symbol table.
 *
 * @param bits the amount of bits the implementation shall handle
 */
#define elfFile_parseSymtab(bits)                                                                                \
static inline bool elfFile_parseSymtab##bits(struct elfFile*   self,                                             \
                                             Elf##bits##_Shdr* symtab,                                           \
                                             char*             strBegin,                                         \
                                             const void*       begin,                                            \
                                             bool              littleEndian) {                                   \
    const void* symtabBegin = begin + ELF_TO_HOST(bits, symtab->sh_offset, littleEndian);                        \
    uint64_t count = ELF_TO_HOST(bits, symtab->sh_size, littleEndian) / sizeof(Elf##bits##_Sym);                 \
    const Elf##bits##_Sym* entry = symtabBegin;                                                                  \
    for (uint64_t i = 0; i < count; ++i, ++entry) {                                                              \
        const unsigned char type = ELF##bits##_ST_TYPE(entry->st_info);                                          \
        if ((type == STT_FUNC || type == STT_OBJECT) && ELF_TO_HOST(bits, entry->st_value, littleEndian) != 0) { \
            struct function f = {                                                                                \
                .startAddress = ELF_TO_HOST(bits, entry->st_value, littleEndian),                                \
                .linkedName   = strdup(strBegin + ELF_TO_HOST(32, entry->st_name, littleEndian)),                \
                .length       = ELF_TO_HOST(bits, entry->st_size, littleEndian),                                 \
                .demangledName.has_value = false,                                                                \
            };                                                                                                   \
            vector_push_back(&self->functions, f);                                                               \
        }                                                                                                        \
    }                                                                                                            \
    return true;                                                                                                 \
}

elfFile_parseSymtab(32)
elfFile_parseSymtab(64)

/**
 * Generates an implementation for converting an ELF section to an LCS one.
 *
 * @param bits the amount of bits the implementation shall handle
 */
#define elfFile_sectionToLCSSection(bits)                                                            \
static inline struct lcs_section elfFile_sectionToLCSSection##bits(const void*       buffer,         \
                                                                   Elf##bits##_Shdr* section,        \
                                                                   bool              littleEndian) { \
    return (struct lcs_section) {                                                                    \
        .content = buffer + ELF_TO_HOST(bits, section->sh_offset, littleEndian),                     \
        .size = ELF_TO_HOST(bits, section->sh_size, littleEndian)                                    \
    };                                                                                               \
}

elfFile_sectionToLCSSection(32)
elfFile_sectionToLCSSection(64)

/**
 * Generates an implementation for loading the number of section headers.
 *
 * @param bits the amount of bits the implementation shall handle
 */
#define elfFile_loadShnum(bits) \
static inline uint64_t elfFile_loadShnum##bits(const Elf##bits##_Ehdr* buffer, bool littleEndian) {                                   \
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

/**
 * Generates an implementation for parsing an ELF file.
 *
 * @param bits the amount of bits the implementation shall handle
 */
#define elfFile_parseFileImpl(bits)                                                                                  \
static inline bool elfFile_parseFile##bits(struct elfFile* self, const Elf##bits##_Ehdr* buffer, bool littleEndian) {\
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
        if ((current->sh_flags & SHF_WRITE) != 0 && (current->sh_flags & SHF_ALLOC) != 0) {                          \
            vector_push_back(&self->_.regions, ((pair_ptr_t) {                                                       \
                self->_.relocationOffset + current->sh_addr,                                                         \
                self->_.relocationOffset + current->sh_addr + current->sh_size                                       \
            }));                                                                                                     \
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
 * Generates an implementation for loading the ELF program header number.
 *
 * @param bits the amount of bits the implementation shall handle
 */
#define elfFile_loadEPHNum(bits)                                                                     \
static inline uint32_t elfFile_loadEPHNum##bits(const Elf##bits##_Ehdr* header, bool littleEndian) { \
    const uint16_t e_phnum = ELF_TO_HOST(16, header->e_phnum, littleEndian);                         \
    if (e_phnum != PN_XNUM) {                                                                        \
        return e_phnum;                                                                              \
    }                                                                                                \
                                                                                                     \
    Elf##bits##_Shdr* sect = ((void*) header) + ELF_TO_HOST(bits, header->e_shoff, littleEndian);    \
    return ELF_TO_HOST(32, sect->sh_info, littleEndian);                                             \
}

elfFile_loadEPHNum(32)
elfFile_loadEPHNum(64)

/**
 * Generates an implementation for loading a given ELF file.
 *
 * @param bits the amount of bits the implementation shall handle
 */
#define elfFile_loadELF_impl(bits)                                                                  \
static inline bool elfFile_loadELF##bits(struct elfFile* self, const void* base,                    \
                                         const bool littleEndian) {                                 \
    const Elf##bits##_Ehdr* header = base;                                                          \
                                                                                                    \
    const void* biggest = NULL;                                                                     \
    const uint32_t e_phnum = elfFile_loadEPHNum##bits(header, littleEndian);                        \
    for (uint32_t i = 0; i < e_phnum; ++i) {                                                        \
        Elf##bits##_Phdr* seg = ((void*) header) + ELF_TO_HOST(bits, header->e_phoff, littleEndian) \
                                + i * ELF_TO_HOST(16, header->e_phentsize, littleEndian);           \
        const void* address = base + ELF_TO_HOST(bits, seg->p_offset, littleEndian)                 \
                             + ELF_TO_HOST(bits, seg->p_memsz, littleEndian);                       \
        if (biggest == NULL || biggest < address) {                                                 \
            biggest = address;                                                                      \
        }                                                                                           \
    }                                                                                               \
    self->_.end = biggest;                                                                          \
    return true;                                                                                    \
}

elfFile_loadELF_impl(32)
elfFile_loadELF_impl(64)

/**
 * Parses the given ELF file into the given abstraction object.
 *
 * @param self the ELF file abstraction object
 * @param header the start pointer of the ELF file
 * @return whether the file was parsed successfully
 */
static inline bool elfFile_parseFile(struct elfFile* self, const Elf32_Ehdr* header, const bool shallow) {
    bool success = false;
    const bool littleEndian = header->e_ident[EI_DATA] == ELFDATA2LSB;
    switch (header->e_ident[EI_CLASS]) {
        case ELFCLASS32:
            success = shallow ? elfFile_loadELF32(self, header, littleEndian)
                              : elfFile_parseFile32(self, header, littleEndian);
            break;
            
        case ELFCLASS64:
            success = shallow ? elfFile_loadELF64(self, header, littleEndian)
                              : elfFile_parseFile64(self, (void*) header, littleEndian);
            break;

        default: break;
    }

    if (!shallow && success && self->debugLine.size > 0) {
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
 * @return @c 0 if the two functions compare equal or a value smaller or
 * greater than @c 0 according to the sorting order
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
 * @return @c 0 if the two infos compare equal or a value smaller or greater
 * than @c 0 according to the sorting order
 */
static inline int elfFile_lineInfoCompare(const void* lhs, const void* rhs) {
    const struct dwarf_lineInfo* a = lhs,
                               * b = rhs;

    if (a->address < b->address) return +1;
    if (a->address > b->address) return -1;

    return 0;
}

static inline bool elfFile_parseFileComplete(struct elfFile* self, const Elf32_Ehdr* header) {
    return elfFile_parseFile(self, header, false);
}

/**
 * Loads the ELF file represented by the given abstraction object.
 *
 * @param self the ELF file abstraction object
 * @return whether the ELF file was loaded successfully
 */
bool elfFile_parse(struct elfFile* self) {
    const bool success = loader_loadFileAndExecute(self->_.fileName.original, (union loader_parserFunction) {
        .parseFunc = (loader_parser) elfFile_parseFileComplete
    }, false, self);
    if (success) {
        vector_sort(&self->functions, elfFile_functionCompare);
        vector_sort(&self->lineInfos, elfFile_lineInfoCompare);
    } else {
        vector_destroyWithPtr(&self->functions, function_destroy);
        vector_init(&self->functions);
    }
    return success;
}

bool elfFile_parseShallow(struct elfFile* self) {
    return elfFile_parseFile(self, self->_.startAddress, true);
}

/**
 * Deducts the debugging information available for the given address in the
 * given ELF file abstraction object.
 *
 * @param self the ELF file abstraction object to be searched
 * @param address the address to be translated
 * @return the optionally available debug information
 */
static inline optional_debugInfo_t elfFile_getDebugInfo(const struct elfFile* self, const void* address) {
    optional_debugInfo_t toReturn = { .has_value = false };

    const uint64_t translated = (uintptr_t) address - self->_.relocationOffset;
    const struct function tmp = (struct function) { .startAddress = translated };
    const struct function* closest = upper_bound(&tmp,
                                                 self->functions.content,
                                                 self->functions.count,
                                                 sizeof(struct function),
                                                 elfFile_functionCompare);
    if (closest == NULL
        || closest->startAddress > translated
        || closest->startAddress + closest->length < translated) {
        return toReturn;
    }
    if (!closest->demangledName.has_value) {
        struct function* mutableClosest = (struct function*) closest;
        mutableClosest->demangledName = (optional_string_t) {
            true,
            callstack_parser_demangleCopy(closest->linkedName, false),
        };
    }
    toReturn = (optional_debugInfo_t) {
        .has_value = true,
        .value = (struct debugInfo) {
            .function = *closest,
            .sourceFileInfo = { .has_value = false }
        }
    };

    const struct dwarf_lineInfo tmpInfo = (struct dwarf_lineInfo) { .address = translated };
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
    if (closestInfo->sourceFile.fileName != NULL && closestInfo->sourceFile.fileNameRelative == NULL && closestInfo->sourceFile.fileNameAbsolute == NULL) {
        struct dwarf_lineInfo* mutableClosest = (struct dwarf_lineInfo*) closestInfo;
        mutableClosest->sourceFile.fileNameRelative = path_toRelativePath(closestInfo->sourceFile.fileName);
        mutableClosest->sourceFile.fileNameAbsolute = path_toAbsolutePath(closestInfo->sourceFile.fileName);
    }
    toReturn.value.sourceFileInfo = (optional_sourceFileInfo_t) {
        .has_value = true,
        .value = {
            closestInfo->line,
            closestInfo->column,
            closestInfo->sourceFile.fileName,
            closestInfo->sourceFile.fileNameRelative,
            closestInfo->sourceFile.fileNameAbsolute,
            binaryFile_isOutdated(closestInfo->sourceFile)
        }
    };
    return toReturn;
}

bool elfFile_getFunctionInfo(struct elfFile* self, const char* functionName, struct functionInfo* info) {
    if (!BINARY_FILE_SUPER(self, maybeParse)) {
        return false;
    }

    vector_iterate(&self->functions, {
        if (strcmp(element->linkedName, functionName) == 0) {
            info->begin = (uintptr_t) element->startAddress + self->_.relocationOffset;
            info->length = element->length;
            return true;
        }
    });
    return false;
}

vector_pair_ptr_t elfFile_getTLSRegions(struct elfFile* self) {
    // TODO: Properly implement
    (void) self;
    return (vector_pair_ptr_t) vector_initializer;
}

bool elfFile_addr2String(struct elfFile* self, const void* address, struct callstack_frame* frame) {
    if (!BINARY_FILE_SUPER(self, maybeParse)) {
        return false;
    }

    const optional_debugInfo_t result = elfFile_getDebugInfo(self, address);
    if (result.has_value) {
        if (result.value.function.linkedName == NULL) {
            return false;
        }
        const char* name = callstack_rawNames || result.value.function.demangledName.value == NULL
            ? result.value.function.linkedName : result.value.function.demangledName.value;
        if (result.value.sourceFileInfo.has_value) {
            frame->sourceFile = utils_maybeCopySave(result.value.sourceFileInfo.value.sourceFileAbsolute, !frame->reserved1);
            frame->sourceFileRelative = utils_maybeCopySave(result.value.sourceFileInfo.value.sourceFileRelative, !frame->reserved1);
            frame->sourceFileOutdated = result.value.sourceFileInfo.value.outdated;
            frame->sourceLine = result.value.sourceFileInfo.value.line;
            frame->sourceLineColumn = result.value.sourceFileInfo.value.column;
            frame->function = utils_maybeCopySave(name, !frame->reserved1);
            frame->reserved2 = frame->reserved1;
        } else {
            char* toReturn = NULL;
            asprintf(&toReturn, "%s + %td", name, (ptrdiff_t) (address - self->_.relocationOffset - result.value.function.startAddress));
            frame->function = toReturn;
            frame->reserved2 = false;
        }
        return true;
    }
    return false;
}

void elfFile_destroy(struct elfFile* self) {
    vector_destroyWithPtr(&self->functions, function_destroy);
    vector_destroyWith(&self->lineInfos, dwarf_lineInfo_destroyValue);
}

void elfFile_delete(struct elfFile* self) {
    BINARY_FILE_SUPER(self, destroy);
    free(self);
}
