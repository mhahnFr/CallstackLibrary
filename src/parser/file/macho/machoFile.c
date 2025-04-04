/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <mach-o/loader.h>

#include <macho/fat_handler.h>
#include <macho/macho_utils.h>
#include <misc/string_utils.h>

#include "cache.h"
#include "machoFile.h"
#include "macho_parser.h"

#include "../bounds.h"
#include "../loader.h"
#include "../dwarf/leb128.h"

#include "../../callstack_parser.h"

struct machoFile* machoFile_new(void)  {
    struct machoFile* toReturn = malloc(sizeof(struct machoFile));
    
    if (toReturn != NULL) {
        machoFile_create(toReturn);
    }
    return toReturn;
}

void machoFile_create(struct machoFile* self) {
    BINARY_FILE_SUPER_1(self, create);

    self->addressOffset    = 0;
    self->linkedit_fileoff = 0;
    self->text_vmaddr      = 0;
    self->linkedit_vmaddr  = 0;

    self->dSYMFile.triedParsing = false;
    self->dSYMFile.file         = NULL;

    vector_init(&self->functions);
    vector_init(&self->functionStarts);
}

/**
 * Returns an object file abstraction object representing the dSYM DWARF file of the given Mach-O file.
 *
 * @param self the Mach-O file abstraction object
 * @return the object file object or `NULL` if either no dSYM bundle was found or the allocation failed
 */
static inline struct objectFile* machoFile_findDSYMBundle(struct machoFile* self) {
    const char* const dsymAmendment = ".dSYM/Contents/Resources/DWARF/";
    const char* rawName = strrchr(self->_.fileName, '/');
    if (rawName == NULL) return NULL;
    rawName++;
    const size_t size = strlen(self->_.fileName) + strlen(dsymAmendment) + strlen(rawName) + 1;
    char* name = malloc(size);
    if (name == NULL) return NULL;
    strlcpy(name, self->_.fileName, size);
    strlcat(name, dsymAmendment, size);
    strlcat(name, rawName, size);
    name[size - 1] = '\0';

    struct stat s;
    if (stat(name, &s) != 0) {
        free(name);
        return NULL;
    }

    struct objectFile* toReturn = objectFile_new();
    if (toReturn == NULL) return NULL;

    toReturn->name         = name;
    toReturn->isDsymBundle = true;
    return toReturn;
}

/**
 * Returns the associated debug symbol bundle of the given Mach-O file abstraction object.
 *
 * @param self the Mach-O file whose debug symbol bundle to be returned
 * @return the debug symbol bundle as object file abstraction object or `NULL` if not found or unable to allocate
 */
static inline struct objectFile* machoFile_getDSYMBundle(struct machoFile* self) {
    if (!self->dSYMFile.triedParsing) {
        self->dSYMFile.file = machoFile_findDSYMBundle(self);
        self->dSYMFile.triedParsing = true;
    }
    return self->dSYMFile.file;
}

/**
 * @brief Returns how the two given function / object file pairs compare.
 *
 * Sorted descendingly.
 *
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return `0` if the two values compare equal, a value smaller or greater than `0` according to the sorting order
 */
static inline int machoFile_functionSortCompare(const void* lhs, const void* rhs) {
    const pair_funcFile_t* a = lhs;
    const pair_funcFile_t* b = rhs;

    if (a->first.startAddress < b->first.startAddress) return +1;
    if (a->first.startAddress > b->first.startAddress) return -1;

    if (a->second == NULL && b->second != NULL) return +1;
    if (a->second != NULL && b->second == NULL) return -1;

    return 0;
}

/**
 * Tries to deduct the debugging information available for the given address in the given Mach-O file.
 *
 * @param self the Mach-O file to search the debug information in
 * @param address the address to be translated
 * @return the optionally deducted debug information
 */
static inline optional_debugInfo_t machoFile_getDebugInfo(struct machoFile* self, void* address) {
    const uint64_t searchAddress = (uintptr_t) (address - self->_.startAddress)
                                 + (self->_.inMemory ? self->text_vmaddr : self->addressOffset);

    const pair_funcFile_t tmp = (pair_funcFile_t) { .first.startAddress = searchAddress };
    const pair_funcFile_t* closest = lower_bound(&tmp,
                                                 self->functions.content,
                                                 self->functions.count,
                                                 sizeof(pair_funcFile_t),
                                                 machoFile_functionSortCompare);

    if (closest == NULL
        || (closest->first.length != 0 && closest->first.startAddress + closest->first.length < searchAddress)) {
        return (optional_debugInfo_t) { .has_value = false };
    }
    optional_debugInfo_t info = { .has_value = false };
    if (!closest->first.demangledName.has_value) {
        pair_funcFile_t* mutableClosest = (struct pair_funcFile*) closest;
        char* toDemangle = closest->first.linkedName;
        if (*toDemangle == '\1' || *toDemangle == '_') {
            ++toDemangle;
        }
        mutableClosest->first.demangledName = (optional_string_t) {
            true,
            callstack_parser_demangleCopy(toDemangle, false),
        };
    }
    if (machoFile_getDSYMBundle(self) != NULL && memcmp(self->uuid, objectFile_getUUID(self->dSYMFile.file), 16) == 0) {
        info = objectFile_getDebugInfo(self->dSYMFile.file, searchAddress, closest->first);
        if (info.has_value) {
            return info;
        }
    }
    if (closest->second == NULL) {
        return (optional_debugInfo_t) {
            true, (struct debugInfo) {
                closest->first,
                .sourceFileInfo.has_value = false
            }
        };
    }
    info = objectFile_getDebugInfo(closest->second, searchAddress, closest->first);
    if (!info.has_value) {
        info = (optional_debugInfo_t) {
            true, (struct debugInfo) {
                closest->first,
                .sourceFileInfo.has_value = false
            }
        };
    }
    return info;
}

#define machoFile_handleSegment(type, bits)                                                                        \
static inline bool machoFile_handleSegment##bits(struct machoFile* self, type* segment, bool bytesSwapped) {       \
    if (strcmp(segment->segname, SEG_PAGEZERO) == 0) {                                                             \
        self->addressOffset = macho_maybeSwap(bits, bytesSwapped, segment->vmaddr)                                 \
                            + macho_maybeSwap(bits, bytesSwapped, segment->vmsize);                                \
    } else if (strcmp(segment->segname, SEG_LINKEDIT) == 0) {                                                      \
        self->linkedit_vmaddr  = macho_maybeSwap(bits, bytesSwapped, segment->vmaddr);                             \
        self->linkedit_fileoff = macho_maybeSwap(bits, bytesSwapped, segment->fileoff);                            \
    } else if (strcmp(segment->segname, SEG_TEXT) == 0) {                                                          \
        self->text_vmaddr = macho_maybeSwap(bits, bytesSwapped, segment->vmaddr);                                  \
    }                                                                                                              \
                                                                                                                   \
    if (segment->initprot & 2 && segment->initprot & 1) {                                                          \
        vector_push_back(&self->_.regions, ((pair_ptr_t) { segment->vmaddr, segment->vmaddr + segment->vmsize })); \
    }                                                                                                              \
                                                                                                                   \
    return true;                                                                                                   \
}

machoFile_handleSegment(struct segment_command,    32)
machoFile_handleSegment(struct segment_command_64, 64)

/**
 * Adds the given function / object file pair to the Mach-O file abstraction object passed via the `va_list`.
 *
 * @param function the function / object file object pair
 * @param args the argument list
 */
static inline void machoFile_addFunction(struct pair_funcFile function, va_list args) {
    struct machoFile* self = va_arg(args, void*);

    vector_push_back(&self->functions, function);
}

/**
 * Compares the given @c uint64_t numbers.
 *
 * @param lhs the left hand side number
 * @param rhs the right hand side number
 * @return the comparasion result, that is, `lhs - rhs`
 */
static inline int machoFile_uint64Compare(uint64_t* lhs, uint64_t* rhs) {
    if (*lhs == *rhs) return 0;

    return *lhs < *rhs ? -1 : +1;
}

/**
 * Handles the function starts Mach-O command.
 *
 * @param self the Mach-O file abstraction structure
 * @param command the Mach-O data command
 * @param baseAddress the start address of the runtime image
 * @param bitsReversed whether to swap the endianess of read numbers
 * @return whether the parsing was successful
 */
static inline bool machoFile_handleFunctionStarts(struct machoFile* self, struct linkedit_data_command* command,
                                                  const void* baseAddress, bool bitsReversed) {
    uint32_t offset = macho_maybeSwap(32, bitsReversed, command->dataoff);
    uint32_t size   = macho_maybeSwap(32, bitsReversed, command->datasize);

    const void* bytes = baseAddress + offset + (self->_.inMemory ? (self->linkedit_vmaddr - self->text_vmaddr) - self->linkedit_fileoff : 0);
    uint64_t funcAddr = self->text_vmaddr; // <--- TODO: What is the appropriate start when read from disk?
    for (size_t i = 0; i < size;) {
        funcAddr += getULEB128(bytes, &i);
        vector_push_back(&self->functionStarts, funcAddr);
    }
    vector_sort(&self->functionStarts, &machoFile_uint64Compare);
    return true;
}

/**
 * Adds the function length to the functions that do not have this information.
 *
 * @param self the Mach-O file abstraction structure
 */
static inline void machoFile_fixupFunctions(struct machoFile* self) {
    vector_iterate(&self->functions, {
        if (element->first.length != 0) continue;

        uint64_t* address = vector_search(&self->functionStarts, &element->first.startAddress, &machoFile_uint64Compare);
        if (address != NULL && (size_t) (address - self->functionStarts.content) < self->functionStarts.count - 2) {
            element->first.length = *++address - element->first.startAddress;
        }
    });
}

#define machoFile_parseFileImpl(bits, type, segMacro)                                                                  \
static inline bool machoFile_parseFileImpl##bits(struct machoFile* self, const void* baseAddress, bool bytesSwapped) { \
    const struct type*   header = baseAddress;                                                                         \
    struct load_command* lc     = (void *) header + sizeof(struct type);                                               \
    const  uint32_t      ncmds  = macho_maybeSwap(32, bytesSwapped, header->ncmds);                                    \
                                                                                                                       \
    for (size_t i = 0; i < ncmds; ++i) {                                                                               \
        bool result = true;                                                                                            \
        switch (macho_maybeSwap(32, bytesSwapped, lc->cmd)) {                                                          \
            case segMacro:                                                                                             \
                result = machoFile_handleSegment##bits(self, (void *) lc, bytesSwapped);                               \
                break;                                                                                                 \
                                                                                                                       \
            case LC_SYMTAB:                                                                                            \
                result = macho_parseSymtab((void*) lc, baseAddress,                                                    \
                                           self->_.inMemory ? (self->linkedit_vmaddr - self->text_vmaddr)              \
                                                              - self->linkedit_fileoff                                 \
                                                            : 0,                                                       \
                                           bytesSwapped, bits == 64, NULL, machoFile_addFunction, self);               \
                break;                                                                                                 \
                                                                                                                       \
            case LC_UUID:                                                                                              \
                memcpy(&self->uuid, &((struct uuid_command*) ((void*) lc))->uuid, 16);                                 \
                result = true;                                                                                         \
                break;                                                                                                 \
                                                                                                                       \
            case LC_FUNCTION_STARTS:                                                                                   \
                result = machoFile_handleFunctionStarts(self, (void*) lc, baseAddress, bytesSwapped);                  \
                break;                                                                                                 \
        }                                                                                                              \
        if (!result) {                                                                                                 \
            return false;                                                                                              \
        }                                                                                                              \
        lc = (void *) lc + macho_maybeSwap(32, bytesSwapped, lc->cmdsize);                                             \
    }                                                                                                                  \
                                                                                                                       \
    machoFile_fixupFunctions(self);                                                                                    \
                                                                                                                       \
    intptr_t diff = (uintptr_t) header - self->text_vmaddr;                                                            \
    vector_iterate(&self->_.regions, {                                                                                 \
        element->first += diff;                                                                                        \
        element->second += diff;                                                                                       \
    });                                                                                                                \
                                                                                                                       \
    return true;                                                                                                       \
}

machoFile_parseFileImpl(32, mach_header,    LC_SEGMENT)
machoFile_parseFileImpl(64, mach_header_64, LC_SEGMENT_64)

/**
 * Parses the given Mach-O file buffer into the given Mach-O file abstraction object.
 *
 * @param self the Mach-O file abstraction object
 * @param baseAddress the Mach-O file buffer
 * @return whether the parsing was successful
 */
static inline bool machoFile_parseFile(struct machoFile* self, const void* baseAddress) {
    if (baseAddress == NULL) return false;

    const struct mach_header* header = baseAddress;
    switch (header->magic) {
        case MH_MAGIC:    return machoFile_parseFileImpl32(self, baseAddress, false);
        case MH_CIGAM:    return machoFile_parseFileImpl32(self, baseAddress, true);
        case MH_MAGIC_64: return machoFile_parseFileImpl64(self, baseAddress, false);
        case MH_CIGAM_64: return machoFile_parseFileImpl64(self, baseAddress, true);

        case FAT_MAGIC:
        case FAT_MAGIC_64: return machoFile_parseFile(self, macho_parseFat(baseAddress, false, self->_.fileName));

        case FAT_CIGAM:
        case FAT_CIGAM_64: return machoFile_parseFile(self, macho_parseFat(baseAddress, true, self->_.fileName));
    }
    return false;
}

bool machoFile_parse(struct machoFile* self) {
    const bool success = self->_.inMemory ? machoFile_parseFile(self, self->_.startAddress)
                                        : loader_loadFileAndExecute(self->_.fileName,
                                                                    (union loader_parserFunction) { (loader_parser) machoFile_parseFile },
                                                                    false,
                                                                    self);
    if (success) {
        vector_sort(&self->functions, machoFile_functionSortCompare);
    } else {
        vector_iterate(&self->functions, function_destroy(&element->first););
        vector_clear(&self->functions);
    }

    return success;
}

bool machoFile_getFunctionInfo(struct machoFile* self, const char* functionName, struct functionInfo* info) {
    if (!BINARY_FILE_SUPER_1(self, maybeParse)) {
        return false;
    }

    vector_iterate(&self->functions, {
        if (strcmp(element->first.linkedName, functionName) == 0) {
            info->begin = (uintptr_t) element->first.startAddress + (uintptr_t) self->_.startAddress
                        - (self->_.inMemory ? self->text_vmaddr : self->addressOffset);
            info->length = element->first.length;
            return true;
        }
    });
    return false;
}

bool machoFile_addr2String(struct machoFile* self, void* address, struct callstack_frame* frame) {
    if (!BINARY_FILE_SUPER_1(self, maybeParse)) {
        return false;
    }

    optional_debugInfo_t result = machoFile_getDebugInfo(self, address);
    if (result.has_value) {
        if (result.value.function.linkedName == NULL) {
            return false;
        }

        const char* name;
        if (callstack_rawNames) {
            name = result.value.function.linkedName;
            if (*name == '\1') {
                ++name;
            }
        } else {
            if (result.value.function.demangledName.value == NULL) {
                name = result.value.function.linkedName;
                if (*name == '_' || *name == '\1') {
                    ++name;
                }
            } else {
                name = result.value.function.demangledName.value;
            }
        }

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
            asprintf(&toReturn, "%s + %td",
                     name,
                     (ptrdiff_t) (address - self->_.startAddress
                                  + (self->_.inMemory ? self->text_vmaddr : self->addressOffset)
                                  - result.value.function.startAddress));
            frame->function = toReturn;
            frame->reserved2 = false;
        }
        return true;
    }
    return false;
}

void machoFile_destroy(struct machoFile* self) {
    vector_iterate(&self->functions, function_destroy(&element->first););
    vector_destroy(&self->functions);
    if (self->dSYMFile.file != NULL) {
        objectFile_delete(self->dSYMFile.file);
    }
    vector_destroy(&self->functionStarts);
}

void machoFile_delete(struct machoFile* self) {
    machoFile_destroy(self);
    free(self);
}

void machoFile_clearCaches(void) {
    macho_cache_destroy();
}
