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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <mach-o/loader.h>

#include <file/pathUtils.h>
#include <macho/fat_handler.h>
#include <macho/macho_utils.h>

#include "cache.h"
#include "machoFile.h"
#include "macho_parser.h"

#include "../bounds.h"
#include "../loader.h"

#include "../../callstack_parser.h"

struct machoFile* machoFile_new(const char* fileName)  {
    struct machoFile* toReturn = malloc(sizeof(struct machoFile));
    
    if (toReturn != NULL) {
        machoFile_create(toReturn, fileName);
    }
    return toReturn;
}

void machoFile_create(struct machoFile* self, const char* fileName) {
    binaryFile_create(&self->_);

    self->_.type     = MACHO_FILE;
    self->_.concrete = self;
    self->_.fileName = fileName;

    self->_.addr2String = &machoFile_addr2String;
    self->_.destroy     = &machoFile_destroy;
    self->_.deleter     = &machoFile_delete;

    self->addressOffset    = 0;
    self->linkedit_fileoff = 0;
    self->text_vmaddr      = 0;
    self->linkedit_vmaddr  = 0;

    self->dSYMFile.triedParsing = false;
    self->dSYMFile.file         = NULL;

    vector_pairFuncFile_create(&self->functions);
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
 * Handles the given segment command.
 *
 * @param self the Mach-O file abstraction object
 * @param segment the segment command to handle
 * @param bitsReversed whether to swap the numbers' endianness
 * @return whether the handling was successful
 */
static inline bool machoFile_handleSegment(struct machoFile *       self,
                                           struct segment_command * segment,
                                           bool                     bitsReversed) {
    if (strcmp(segment->segname, SEG_PAGEZERO) == 0) {
        self->addressOffset = macho_maybeSwap(32, bitsReversed, segment->vmaddr)
                            + macho_maybeSwap(32, bitsReversed, segment->vmsize);
    } else if (strcmp(segment->segname, SEG_LINKEDIT) == 0) {
        self->linkedit_vmaddr  = macho_maybeSwap(32, bitsReversed, segment->vmaddr);
        self->linkedit_fileoff = macho_maybeSwap(32, bitsReversed, segment->fileoff);
    } else if (strcmp(segment->segname, SEG_TEXT) == 0) {
        self->text_vmaddr = macho_maybeSwap(32, bitsReversed, segment->vmaddr);
    }
    return true;
}

/**
 * Handles the given 64 bit segment command.
 *
 * @param self the Mach-O file abstraction object
 * @param segment the 64 bit segment command to handle
 * @param bitsReversed whether to swap the numbers' endianness
 * @return whether the handling was successfully
 */
static inline bool machoFile_handleSegment64(struct machoFile *          self,
                                             struct segment_command_64 * segment,
                                             bool                        bitsReversed) {
    if (strcmp(segment->segname, SEG_PAGEZERO) == 0) {
        self->addressOffset = macho_maybeSwap(64, bitsReversed, segment->vmaddr)
                            + macho_maybeSwap(64, bitsReversed, segment->vmsize);
    } else if (strcmp(segment->segname, SEG_LINKEDIT) == 0) {
        self->linkedit_vmaddr  = macho_maybeSwap(64, bitsReversed, segment->vmaddr);
        self->linkedit_fileoff = macho_maybeSwap(64, bitsReversed, segment->fileoff);
    } else if (strcmp(segment->segname, SEG_TEXT) == 0) {
        self->text_vmaddr = macho_maybeSwap(64, bitsReversed, segment->vmaddr);
    }
    return true;
}

/**
 * Adds the given function / object file pair to the Mach-O file abstraction object passed via the `va_list`.
 *
 * @param function the function / object file object pair
 * @param args the argument list
 */
static inline void machoFile_addFunction(struct pair_funcFile function, va_list args) {
    struct machoFile* self = va_arg(args, void*);

    vector_pairFuncFile_push_back(&self->functions, function);
}

/**
 * Parses a Mach-O file into the given Mach-O file abstraction object.
 *
 * @param self the Mach-O file abstraction object
 * @param baseAddress the base address of the Mach-O file
 * @param bitsReversed whether to swap the numbers' endianess
 * @return whether the parsing was successfull
 */
static inline bool machoFile_parseFileImpl(struct machoFile * self,
                                           const void*        baseAddress,
                                           bool               bitsReversed) {
    const struct mach_header* header = baseAddress;
    struct load_command * lc     = (void *) header + sizeof(struct mach_header);
    const  uint32_t       ncmds  = macho_maybeSwap(32, bitsReversed, header->ncmds);

    for (size_t i = 0; i < ncmds; ++i) {
        bool result = true;
        switch (macho_maybeSwap(32, bitsReversed, lc->cmd)) {
            case LC_SEGMENT:
                result = machoFile_handleSegment(self, (void *) lc, bitsReversed);
                break;

            case LC_SYMTAB:
                result = macho_parseSymtab((void*) lc, baseAddress,
                                           self->_.inMemory ? (self->linkedit_vmaddr - self->text_vmaddr) - self->linkedit_fileoff : 0,
                                           bitsReversed, false, NULL, machoFile_addFunction, self);
                break;

            case LC_UUID:
                memcpy(&self->uuid, &((struct uuid_command*) ((void*) lc))->uuid, 16);
                result = true;
                break;
        }
        if (!result) {
            return false;
        }
        lc = (void *) lc + macho_maybeSwap(32, bitsReversed, lc->cmdsize);
    }
    return true;
}

/**
 * Parses a 64 bit Mach-O file into the given Mach-O file abstraction object.
 *
 * @param self the Mach-O file abstraction object
 * @param baseAddress the base address of the Mach-O file
 * @param bitsReversed whether to swap the numbers' endianess
 * @return whether the parsing was successfull
 */
static inline bool machoFile_parseFileImpl64(struct machoFile * self,
                                             const void*        baseAddress,
                                             bool               bitsReversed) {
    const struct mach_header_64* header = baseAddress;
    struct load_command *   lc     = (void *) header + sizeof(struct mach_header_64);
    const  uint32_t         ncmds  = macho_maybeSwap(32, bitsReversed, header->ncmds);

    for (size_t i = 0; i < ncmds; ++i) {
        bool result = true;
        switch (macho_maybeSwap(32, bitsReversed, lc->cmd)) {
            case LC_SEGMENT_64:
                result = machoFile_handleSegment64(self, (void *) lc, bitsReversed);
                break;

            case LC_SYMTAB:
                result = macho_parseSymtab((void*) lc, baseAddress,
                                           self->_.inMemory ? (self->linkedit_vmaddr - self->text_vmaddr) - self->linkedit_fileoff : 0,
                                           bitsReversed, true, NULL, machoFile_addFunction, self);
                break;

            case LC_UUID:
                memcpy(&self->uuid, &((struct uuid_command*) ((void*) lc))->uuid, 16);
                result = true;
                break;
        }
        if (!result) {
            return false;
        }
        lc = (void *) lc + macho_maybeSwap(32, bitsReversed, lc->cmdsize);
    }
    return true;
}

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
        case MH_MAGIC:    return machoFile_parseFileImpl(self, baseAddress, false);
        case MH_CIGAM:    return machoFile_parseFileImpl(self, baseAddress, true);
        case MH_MAGIC_64: return machoFile_parseFileImpl64(self, baseAddress, false);
        case MH_CIGAM_64: return machoFile_parseFileImpl64(self, baseAddress, true);

        case FAT_MAGIC:
        case FAT_MAGIC_64: return machoFile_parseFile(self, macho_parseFat(baseAddress, false, self->_.fileName));

        case FAT_CIGAM:
        case FAT_CIGAM_64: return machoFile_parseFile(self, macho_parseFat(baseAddress, true, self->_.fileName));
    }
    return false;
}

/**
 * Loads and parses the Mach-O file represented by the given Mach-O file abstraction object.
 *
 * @param self the Mach-O file abstraction object
 * @return whether the file was parsed successfully
 */
static inline bool machoFile_loadFile(struct machoFile* self) {
    const bool success = self->_.inMemory ? machoFile_parseFile(self, self->_.startAddress)
                                        : loader_loadFileAndExecute(self->_.fileName,
                                                                    (union loader_parserFunction) { (loader_parser) machoFile_parseFile },
                                                                    false,
                                                                    self);
    if (success) {
        qsort(self->functions.content, self->functions.count, sizeof(pair_funcFile_t), machoFile_functionSortCompare);
    } else {
        vector_iterate(pair_funcFile_t, &self->functions, function_destroy(&element->first);)
        vector_pairFuncFile_clear(&self->functions);
    }

    return success;
}

bool machoFile_addr2String(struct binaryFile* me, void* address, struct callstack_frame* frame) {
    struct machoFile * self = machoFileOrNull(me);
    if (self == NULL) {
        return false;
    }
    if (!self->_.parsed &&
        !(self->_.parsed = machoFile_loadFile(self))) {
        return false;
    }

    optional_debugInfo_t result = machoFile_getDebugInfo(self, address);
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
            frame->sourceFile = path_toAbsolutePath((char*) result.value.sourceFileInfo.value.sourceFile);
            frame->sourceFileRelative = path_toRelativePath((char*) result.value.sourceFileInfo.value.sourceFile);
            frame->sourceFileOutdated = result.value.sourceFileInfo.value.outdated;
            frame->sourceLine = result.value.sourceFileInfo.value.line;
            frame->sourceLineColumn = result.value.sourceFileInfo.value.column;
            frame->function = name;
        } else {
            char* toReturn = NULL;
            asprintf(&toReturn, "%s + %td",
                     name,
                     (ptrdiff_t) (address - self->_.startAddress
                                  + (self->_.inMemory ? self->text_vmaddr : self->addressOffset)
                                  - result.value.function.startAddress));
            free(name);
            frame->function = toReturn;
        }
        return true;
    }
    return false;
}

void machoFile_destroy(struct binaryFile * me) {
    struct machoFile* self = machoFileOrNull(me);
    if (self == NULL) {
        return;
    }
    
    vector_iterate(pair_funcFile_t, &self->functions, function_destroy(&element->first);)
    vector_pairFuncFile_destroy(&self->functions);
    if (self->dSYMFile.file != NULL) {
        objectFile_delete(self->dSYMFile.file);
    }
}

void machoFile_delete(struct binaryFile* me) {
    me->destroy(me);
    struct machoFile* self = machoFileOrNull(me);
    if (self == NULL) {
        return;
    }
    free(self);
}

void machoFile_clearCaches(void) {
    macho_cache_destroy();
}
