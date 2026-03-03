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

#include "machoFile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach-o/loader.h>
#include <macho/fat_handler.h>
#include <macho/macho_utils.h>
#include <misc/numberContainers.h>
#include <misc/string_utils.h>
#include <sys/stat.h>

#include "cache.h"
#include "macho_parser.h"
#include "../bounds.h"
#include "../dc4c_exceptions.h"
#include "../exception.h"
#include "../loader.h"
#include "../../callstack_parser.h"
#include "../dwarf/leb128.h"

void machoFile_create(struct machoFile* self) {
    self->addressOffset    = 0;
    self->linkedit_fileoff = 0;
    self->text_vmaddr      = 0;
    self->linkedit_vmaddr  = 0;
    self->tlvSize          = 0;

    self->dSYMFile.triedParsing = false;
    self->dSYMFile.file         = NULL;

    vector_init(&self->symbols);
    vector_init(&self->functionStarts);
    vector_init(&self->tlvs);
}

/**
 * Returns an object file abstraction object representing the dSYM DWARF file
 * of the given Mach-O file.
 *
 * @param self the Mach-O file abstraction object
 * @return the object file object or @c NULL if either no dSYM bundle was found
 * or the allocation failed
 */
static inline struct objectFile* machoFile_findDSYMBundle(const struct machoFile* self) {
    const char* const dsymAmendment = ".dSYM/Contents/Resources/DWARF/";
    const char* rawName = strrchr(self->_.fileName.original, '/');
    if (rawName == NULL) return NULL;
    rawName++;
    const size_t size = strlen(self->_.fileName.original) + strlen(dsymAmendment) + strlen(rawName) + 1;
    char* name = malloc(size);
    if (name == NULL) return NULL;
    strlcpy(name, self->_.fileName.original, size);
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
 * Returns the associated debug symbol bundle of the given Mach-O file
 * abstraction object.
 *
 * @param self the Mach-O file whose debug symbol bundle to be returned
 * @return the debug symbol bundle as object file abstraction object or @c NULL
 * if not found or unable to allocate
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
 * @return @c 0 if the two values compare equal, a value smaller or greater
 * than @c 0 according to the sorting order
 */
static inline int machoFile_functionSortCompare(const void* lhs, const void* rhs) {
    const pair_symbolFile_t* a = lhs;
    const pair_symbolFile_t* b = rhs;

    if (a->first.startAddress < b->first.startAddress) return +1;
    if (a->first.startAddress > b->first.startAddress) return -1;

    if (a->second == NULL && b->second != NULL) return +1;
    if (a->second != NULL && b->second == NULL) return -1;

    return 0;
}

/**
 * Tries to deduct the debugging information available for the given address in
 * the given Mach-O file.
 *
 * @param self the Mach-O file to search the debug information in
 * @param address the address to be translated
 * @param function the search function to be used to search for appropriate
 * debug information
 * @return the optionally deducted debug information
 */
static inline optional_debugInfo_t machoFile_getDebugInfo(struct machoFile* self, const void* address,
                                                          const binaryFile_searchFunction function) {
    const uint64_t searchAddress = (uintptr_t) (address - self->_.startAddress)
                                 + (self->_.inMemory ? self->text_vmaddr : self->addressOffset);

    const pair_symbolFile_t tmp = (pair_symbolFile_t) { .first.startAddress = searchAddress };
    const pair_symbolFile_t* closest = function(&tmp,
                                                self->symbols.content,
                                                self->symbols.count,
                                                sizeof(pair_symbolFile_t),
                                                machoFile_functionSortCompare);

    optional_debugInfo_t info = { .has_value = false };
    if (closest == NULL
        || (closest->first.length != 0 && closest->first.startAddress + closest->first.length < searchAddress)) {
        return info;
    }
    if (!closest->first.demangledName.has_value) {
        pair_symbolFile_t* mutableClosest = (struct pair_symbolFile*) closest;
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

/**
 * Generates an implementation for the segment handling of the Mach-O files.
 *
 * @param bits the amount of bits the implementation should be generated for
 * @param suffix the optional suffix to be used for the native types
 */
#define machoFile_handleSegment(bits, suffix)                                                   \
static inline void machoFile_handleSegment##bits(struct machoFile* self, const void* buffer,    \
                                                 const struct segment_command##suffix* segment, \
                                                 const bool bytesSwapped, const bool shallow) { \
    uint##bits##_t vmaddr = macho_maybeSwap(bits, bytesSwapped, segment->vmaddr),               \
                   vmsize = macho_maybeSwap(bits, bytesSwapped, segment->vmsize);               \
    if (strcmp(segment->segname, SEG_PAGEZERO) == 0) {                                          \
        self->addressOffset = vmaddr + vmsize;                                                  \
    } else if (strcmp(segment->segname, SEG_LINKEDIT) == 0) {                                   \
        self->linkedit_vmaddr = vmaddr;                                                         \
        self->linkedit_fileoff = macho_maybeSwap(bits, bytesSwapped, segment->fileoff);         \
    } else if (strcmp(segment->segname, SEG_TEXT) == 0) {                                       \
        self->_.relocationOffset = self->text_vmaddr = vmaddr;                                  \
        self->_.end = self->_.startAddress + vmsize;                                            \
    }                                                                                           \
    if (shallow) {                                                                              \
        return;                                                                                 \
    }                                                                                           \
    if (segment->initprot & 2 && segment->initprot & 1) {                                       \
        vector_push_back(&self->_.regions, ((pair_ptr_t) { vmaddr, vmaddr + vmsize }));         \
    }                                                                                           \
    optional_uint64_t size = { .has_value = false, .value = 0 };                                \
    macho_iterateSections(segment, bytesSwapped, suffix, {                                      \
        uint##bits##_t sectionSize = macho_maybeSwap(bits, bytesSwapped, section->size);        \
        switch (section->flags & SECTION_TYPE) {                                                \
            case S_THREAD_LOCAL_ZEROFILL:                                                       \
            case S_THREAD_LOCAL_REGULAR:                                                        \
                size.has_value = true;                                                          \
                size.value += sectionSize;                                                      \
                break;                                                                          \
                                                                                                \
            case S_THREAD_LOCAL_VARIABLES: if (sectionSize != 0) {                              \
                /* TODO: Can have multiple */                                                   \
                uintptr_t slide = (uintptr_t) buffer - self->text_vmaddr;                       \
                TLVDescriptor* begin = (TLVDescriptor*)                                         \
                    (macho_maybeSwap(bits, bytesSwapped, section->addr) + slide);               \
                const size_t amount = sectionSize / sizeof(TLVDescriptor);                      \
                vector_reserve_throw(&self->tlvs, amount);                                            \
                memcpy(self->tlvs.content, begin, sectionSize);                                 \
                self->tlvs.count = amount;                                                      \
            }                                                                                   \
            break;                                                                              \
        }                                                                                       \
    })                                                                                          \
    if (size.has_value) {                                                                       \
        self->tlvSize = size.value;                                                             \
    }                                                                                           \
}

machoFile_handleSegment(32,)
machoFile_handleSegment(64, _64)

/**
 * Callback function for adding the given symbol and object file pair to the
 * symbol list.
 *
 * @param self the Mach-O file object
 * @param pair the symbol and object file pair to be added
 */
static inline void machoFile_addSymbol(struct machoFile* self, pair_symbolFile_t pair) {
    vector_push_back_throw(&self->symbols, pair);
}

/**
 * Compares the given @c uint64_t numbers.
 *
 * @param lhs the left hand side number
 * @param rhs the right hand side number
 * @return the comparison result
 */
static inline int machoFile_uint64Compare(const uint64_t* lhs, const uint64_t* rhs) {
    if (*lhs == *rhs) return 0;

    return *lhs < *rhs ? -1 : +1;
}

/**
 * Handles the function starts Mach-O command.
 *
 * @param self the Mach-O file abstraction structure
 * @param command the Mach-O data command
 * @param baseAddress the start address of the runtime image
 * @param bitsReversed whether to swap the endianness of read numbers
 * @return whether the parsing was successful
 */
static inline void machoFile_handleFunctionStarts(struct machoFile* self, struct linkedit_data_command* command,
                                                  const void* baseAddress, const bool bitsReversed) {
    const uint32_t offset = macho_maybeSwap(32, bitsReversed, command->dataoff);
    const uint32_t size   = macho_maybeSwap(32, bitsReversed, command->datasize);

    const void* bytes = baseAddress + offset + (self->_.inMemory ? self->linkedit_vmaddr - self->text_vmaddr - self->linkedit_fileoff : 0);
    uint64_t funcAddr = self->text_vmaddr; // <--- TODO: What is the appropriate start when read from disk?
    for (size_t i = 0; i < size;) {
        funcAddr += getULEB128(bytes, &i);
        vector_push_back_throw(&self->functionStarts, funcAddr);
    }
    vector_sort(&self->functionStarts, &machoFile_uint64Compare);
}

/**
 * Adds the function length to the functions that do not have this information.
 *
 * @param self the Mach-O file abstraction structure
 */
static inline void machoFile_fixupFunctions(struct machoFile* self) {
    vector_iterate(&self->symbols, {
        if (element->first.length != 0) continue;

        const uint64_t* address = vector_search(&self->functionStarts, &element->first.startAddress, &machoFile_uint64Compare);
        if (address != NULL && (size_t) (address - self->functionStarts.content) < self->functionStarts.count - 2) {
            element->first.length = *++address - element->first.startAddress;
        }
    });
}

/**
 * Generates an implementation to parse Mach-O files.
 *
 * @param bits the amount of bits the implementation should handle
 * @param suffix the optional suffix to be used for the native data structures
 */
#define machoFile_parseFileImpl(bits, suffix)                                                                 \
static inline void machoFile_parseFileImpl##bits(struct machoFile* self, const void* baseAddress,             \
                                                 const bool bytesSwapped, const bool shallow) {               \
    macho_iterateSegments(baseAddress, bytesSwapped, suffix, {                                                \
        switch (macho_maybeSwap(32, bytesSwapped, loadCommand->cmd)) {                                        \
            case LC_SEGMENT##suffix:                                                                          \
                machoFile_handleSegment##bits(self, baseAddress, (void*) loadCommand, bytesSwapped, shallow); \
                break;                                                                                        \
                                                                                                              \
            case LC_SYMTAB: {                                                                                 \
                if (shallow) {                                                                                \
                    break;                                                                                    \
                }                                                                                             \
                volatile struct machoParser parser = machoParser_create(                                      \
                    (void*) loadCommand, baseAddress, self->_.inMemory ?                                      \
                        (self->linkedit_vmaddr - self->text_vmaddr) - self->linkedit_fileoff : 0,             \
                    bytesSwapped, (bits) == 64, (machoParser_addSymbol) machoFile_addSymbol, self,            \
                    self->_.fileName.original                                                                 \
                );                                                                                            \
                TRY({                                                                                         \
                    machoParser_parseSymbolTable((struct machoParser*) &parser);                              \
                    machoParser_destroy((struct machoParser*) &parser);                                       \
                }, CATCH_ALL(_, {                                                                             \
                    (void) _;                                                                                 \
                    machoParser_destroy((struct machoParser*) &parser);                                       \
                    RETHROW;                                                                                  \
                }))                                                                                           \
                break;                                                                                        \
            }                                                                                                 \
                                                                                                              \
            case LC_UUID:                                                                                     \
                memcpy(&self->uuid, &((struct uuid_command*) ((void*) loadCommand))->uuid, 16);               \
                break;                                                                                        \
                                                                                                              \
            case LC_FUNCTION_STARTS:                                                                          \
                if (!shallow) {                                                                               \
                    machoFile_handleFunctionStarts(self, (void*) loadCommand, baseAddress, bytesSwapped);     \
                }                                                                                             \
                break;                                                                                        \
                                                                                                              \
            default: break;                                                                                   \
        }                                                                                                     \
    })                                                                                                        \
}

machoFile_parseFileImpl(32,)
machoFile_parseFileImpl(64, _64)

/**
 * Parses the given Mach-O file buffer into the given Mach-O file abstraction
 * object.
 *
 * @param self the Mach-O file abstraction object
 * @param baseAddress the Mach-O file buffer
 * @param shallow whether to only parse the strictly necessary information from
 * the represented file
 * @return whether the parsing was successful
 */
static inline void machoFile_parseFile(struct machoFile* self, const void* baseAddress, const bool shallow) {
    if (baseAddress == NULL) {
        BFE_THROW_FILE(empty, self);
    }

    const struct mach_header* header = baseAddress;
    switch (header->magic) {
        case MH_MAGIC:    machoFile_parseFileImpl32(self, baseAddress, false, shallow); break;
        case MH_CIGAM:    machoFile_parseFileImpl32(self, baseAddress, true, shallow);  break;
        case MH_MAGIC_64: machoFile_parseFileImpl64(self, baseAddress, false, shallow); break;
        case MH_CIGAM_64: machoFile_parseFileImpl64(self, baseAddress, true, shallow);  break;

        case FAT_MAGIC:
        case FAT_MAGIC_64: machoFile_parseFile(self, macho_parseFat(baseAddress, false, self->_.fileName.original), shallow); return;

        case FAT_CIGAM:
        case FAT_CIGAM_64: machoFile_parseFile(self, macho_parseFat(baseAddress, true, self->_.fileName.original), shallow); return;

        default: break;
    }
    if (!shallow) {
        machoFile_fixupFunctions(self);

        const uintptr_t diff = (uintptr_t) baseAddress - self->text_vmaddr;
        vector_iterate(&self->_.regions, {
            element->first += diff;
            element->second += diff;
        });
        BINARY_FILE_SUPER(self, sortRegions);
    }
}

/**
 * Parses the given Mach-O file entirely.
 *
 * @param self the Mach-O file object
 * @param baseAddress the base address of the represented file
 * @return whether the parsing was successful
 */
static inline void machoFile_parseFileComplete(struct machoFile* self, const void* baseAddress) {
    machoFile_parseFile(self, baseAddress, false);
}

void machoFile_parseShallow(struct machoFile* self) {
    machoFile_parseFile(self, self->_.startAddress, true);
}

void machoFile_parse(struct machoFile* self) {
    TRY({
        if (self->_.inMemory) {
            machoFile_parseFileComplete(self, self->_.startAddress);
        } else if (!loader_loadFileAndExecute(self->_.fileName.original,
            (union loader_parserFunction) { (loader_parser) machoFile_parseFileComplete },
            false, self)) {
            BFE_THROW_FILE(failed, self);
        }
        vector_sort(&self->symbols, machoFile_functionSortCompare);
    }, CATCH_ALL(_, {
        (void) _;
        vector_iterate(&self->symbols, symbol_destroy(&element->first););
        vector_clear(&self->symbols);
        RETHROW;
    }))
}

bool machoFile_getFunctionInfo(struct machoFile* self, const char* functionName, struct functionInfo* info) {
    if (!BINARY_FILE_SUPER(self, maybeParse)) {
        return false;
    }

    vector_iterate(&self->symbols, {
        if (strcmp(element->first.linkedName, functionName) == 0) {
            info->begin = (uintptr_t) element->first.startAddress + (uintptr_t) self->_.startAddress
                        - (self->_.inMemory ? self->text_vmaddr : self->addressOffset);
            info->length = element->first.length;
            return true;
        }
    });
    return false;
}

vector_pair_ptr_t machoFile_getTLSRegions(struct machoFile* self) {
    if (!BINARY_FILE_SUPER(self, maybeParse)) {
        return (vector_pair_ptr_t) vector_initializer;
    }

    vector_pair_ptr_t toReturn = vector_initializer;
    if (self->tlvs.count > 0) {
        const uintptr_t begin = (uintptr_t) self->tlvs.content->thunk(self->tlvs.content);
        vector_push_back(&toReturn, ((pair_ptr_t) { begin, begin + self->tlvSize }));
    }
    return toReturn;
}

/**
 * Symbolizes the given address into the given @c callstack_frame structure.
 *
 * @param self the Mach-O file object
 * @param address the address to symbolize
 * @param frame the @c callstack_frame structure to be filled
 * @param searchFunction the search function to be used
 * @param forceDiff whether to print a difference value in the symbol name if
 * not found and the difference value would be zero
 * @return whether the symbolization was successful
 */
static inline bool machoFile_addr2StringImpl(struct machoFile* self, const void* address, struct callstack_frame* frame,
                                             const binaryFile_searchFunction searchFunction, const bool forceDiff) {
    if (!BINARY_FILE_SUPER(self, maybeParse)) {
        return false;
    }

    const optional_debugInfo_t result = machoFile_getDebugInfo(self, address, searchFunction);
    if (result.has_value) {
        if (result.value.symbol.linkedName == NULL) {
            return false;
        }

        const char* name;
        if (callstack_rawNames) {
            name = result.value.symbol.linkedName;
            if (*name == '\1') {
                ++name;
            }
        } else {
            if (result.value.symbol.demangledName.value == NULL) {
                name = result.value.symbol.linkedName;
                if (*name == '_' || *name == '\1') {
                    ++name;
                }
            } else {
                name = result.value.symbol.demangledName.value;
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
            const ptrdiff_t diff = (ptrdiff_t) (address - self->_.startAddress
                                      + (self->_.inMemory ? self->text_vmaddr : self->addressOffset)
                                      - result.value.symbol.startAddress);
            if (diff > 0 || forceDiff) {
                asprintf(&toReturn, "%s + %td", name, diff);
                frame->reserved2 = false;
            } else {
                toReturn = utils_maybeCopySave(name, !frame->reserved1);
                frame->reserved2 = frame->reserved1;
            }
            frame->function = toReturn;
        }
        return true;
    }
    return false;
}

bool machoFile_getSymbolInfo(struct machoFile* self, const void* symbolAddress, struct callstack_frame* frame) {
    return machoFile_addr2StringImpl(self, symbolAddress, frame, lower_bound, false);
}

bool machoFile_addr2String(struct machoFile* self, const void* address, struct callstack_frame* frame) {
    return machoFile_addr2StringImpl(self, address, frame, upper_bound, true);
}

void machoFile_destroy(struct machoFile* self) {
    vector_iterate(&self->symbols, symbol_destroy(&element->first););
    vector_destroy(&self->symbols);
    if (self->dSYMFile.file != NULL) {
        objectFile_delete(self->dSYMFile.file);
    }
    vector_destroy(&self->functionStarts);
    vector_destroy(&self->tlvs);
}

void machoFile_delete(struct machoFile* self) {
    BINARY_FILE_SUPER(self, destroy);
    free(self);
}

void machoFile_clearCaches(void) {
    macho_cache_destroy();
}