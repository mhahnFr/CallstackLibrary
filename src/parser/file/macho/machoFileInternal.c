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

#include <string.h>

#include <mach-o/fat.h>
#include <mach-o/loader.h>

#include <Availability.h>

#if defined(__BLOCKS__) && defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && defined(MAC_OS_VERSION_13_0) \
    && __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_13_0
 #define HAS_MACH_O_UTILS
 #include <mach-o/utils.h>
#else
 #include <mach-o/arch.h>
 #include <sys/sysctl.h>
#endif

#include "machoFileInternal.h"

#include "OptionalFunction.h"

#include "macho_parser.h"
#include "macho_utils.h"

/**
 * Handles the given segment command.
 *
 * @param self the Mach-O file abstraction object
 * @param segment the segment command to handle
 * @param bitsReversed whether to swap the numbers' endianess
 * @return whether the handling was successfull
 */
static inline bool machoFile_handleSegment(struct machoFile *       self,
                                           struct segment_command * segment,
                                           bool                     bitsReversed) {
    if (strcmp(segment->segname, SEG_PAGEZERO) == 0) {
        self->addressOffset = macho_maybeSwap(32, bitsReversed, segment->vmaddr)
                            + macho_maybeSwap(32, bitsReversed, segment->vmsize);
    }
    return true;
}

/**
 * Handles the given 64 bit segment command.
 *
 * @param self the Mach-O file abstraction object
 * @param segment the 64 bit segment command to handle
 * @param bitsReversed whether to swap the numbers' endianess
 * @return whether the handling was successfull
 */
static inline bool machoFile_handleSegment64(struct machoFile *          self,
                                             struct segment_command_64 * segment,
                                             bool                        bitsReversed) {
    if (strcmp(segment->segname, SEG_PAGEZERO) == 0) {
        self->addressOffset = macho_maybeSwap(64, bitsReversed, segment->vmaddr)
                            + macho_maybeSwap(64, bitsReversed, segment->vmsize);
    }
    return true;
}

static inline void machoFile_addObjectFileImpl(struct objectFile* file, va_list args) {
    struct machoFile* self = va_arg(args, void*);
    
    machoFile_addObjectFile(self, file);
}

static inline void machoFile_addFunctionImpl(struct function function, va_list args) {
    struct machoFile* self = va_arg(args, void*);

    vector_function_push_back(&self->functions, function);
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
                                           void *             baseAddress,
                                           bool               bitsReversed) {
    struct mach_header *  header = baseAddress;
    struct load_command * lc     = (void *) header + sizeof(struct mach_header);
    const  uint32_t       ncmds  = macho_maybeSwap(32, bitsReversed, header->ncmds);
    
    for (size_t i = 0; i < ncmds; ++i) {
        bool result = true;
        switch (macho_maybeSwap(32, bitsReversed, lc->cmd)) {
            case LC_SEGMENT:
                result = machoFile_handleSegment(self, (void *) lc, bitsReversed);
                break;
                
            case LC_SYMTAB:
                result = macho_parseSymtab((void*) lc, baseAddress, bitsReversed, false, machoFile_addObjectFileImpl, machoFile_addFunctionImpl, self);
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
                                             void *             baseAddress,
                                             bool               bitsReversed) {
    struct mach_header_64 * header = baseAddress;
    struct load_command *   lc     = (void *) header + sizeof(struct mach_header_64);
    const  uint32_t         ncmds  = macho_maybeSwap(32, bitsReversed, header->ncmds);
    
    for (size_t i = 0; i < ncmds; ++i) {
        bool result = true;
        switch (macho_maybeSwap(32, bitsReversed, lc->cmd)) {
            case LC_SEGMENT_64:
                result = machoFile_handleSegment64(self, (void *) lc, bitsReversed);
                break;
                
            case LC_SYMTAB:
                result = macho_parseSymtab((void*) lc, baseAddress, bitsReversed, true, machoFile_addObjectFileImpl, machoFile_addFunctionImpl, self);
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
 * Parses the given fat Mach-O binary file into the given Mach-O file abstraction object.
 *
 * @param self the Mach-O file abstraction object
 * @param fatHeader the fat Mach-O header to parse
 * @param bitsReversed whether to swap the numbers' endianess
 * @return whether the parsing was successfull
 */
static inline bool machoFile_parseFat(struct machoFile *  self,
                                      struct fat_header * fatHeader,
                                      bool                bitsReversed) {
#ifdef HAS_MACH_O_UTILS
    (void) bitsReversed;
    
    __block uint64_t fileOffset;
    
    const int result = macho_best_slice(self->_.fileName,
                                        ^ (const struct mach_header * header, uint64_t offset, size_t size) {
        (void) header;
        (void) size;
        
        fileOffset = offset;
    });
    return result == 0 && machoFile_parseFile(self, (void *) fatHeader + fileOffset);
#else
    uint32_t cputype,
             cpusubtype;
    size_t len = sizeof(uint32_t);
    if (sysctlbyname("hw.cputype", &cputype, &len, NULL, 0) != 0
        || sysctlbyname("hw.cpusubtype", &cpusubtype, &len, NULL, 0) != 0) {
        return false;
    }
    uint64_t offset;
    switch (fatHeader->magic) {
        case FAT_MAGIC_64:
        case FAT_CIGAM_64: {
            const struct fat_arch_64 * best = NXFindBestFatArch_64(macho_maybeSwap(32, bitsReversed, cputype),
                                                                   macho_maybeSwap(32, bitsReversed, cpusubtype),
                                                                   (void *) fatHeader + sizeof(struct fat_header),
                                                                   macho_maybeSwap(32, bitsReversed, fatHeader->nfat_arch));
            if (best == NULL) {
                return false;
            }
            offset = macho_maybeSwap(64, bitsReversed, best->offset);
            break;
        }
            
        case FAT_MAGIC:
        case FAT_CIGAM: {
            const struct fat_arch * best = NXFindBestFatArch(macho_maybeSwap(32, bitsReversed, cputype),
                                                             macho_maybeSwap(32, bitsReversed, cpusubtype),
                                                             (void *) fatHeader + sizeof(struct fat_header),
                                                             macho_maybeSwap(32, bitsReversed, fatHeader->nfat_arch));
            if (best == NULL) {
                return false;
            }
            offset = macho_maybeSwap(32, bitsReversed, best->offset);
            break;
        }
            
        default: return false;
    }
    return machoFile_parseFile(self, (void *) fatHeader + offset);
#endif
}

bool machoFile_parseFile(struct machoFile * self, void * baseAddress) {
    struct mach_header * header = baseAddress;
    switch (header->magic) {
        case MH_MAGIC:    return machoFile_parseFileImpl(self, baseAddress, false);
        case MH_CIGAM:    return machoFile_parseFileImpl(self, baseAddress, true);
        case MH_MAGIC_64: return machoFile_parseFileImpl64(self, baseAddress, false);
        case MH_CIGAM_64: return machoFile_parseFileImpl64(self, baseAddress, true);
            
        case FAT_MAGIC:
        case FAT_MAGIC_64: return machoFile_parseFat(self, baseAddress, false);
            
        case FAT_CIGAM:
        case FAT_CIGAM_64: return machoFile_parseFat(self, baseAddress, true);
    }
    return false;
}

void machoFile_addObjectFile(struct machoFile *  self,
                             struct objectFile * file) {
    file->next        = self->objectFiles;
    self->objectFiles = file;
}

struct optional_funcFile machoFile_findFunction(struct machoFile* self, void* address) {
    struct optional_funcFile toReturn = { .has_value = false };
    
    for (struct objectFile* it = self->objectFiles; it != NULL; it = it->next) {
        struct optional_function result = objectFile_findFunction(it, (uint64_t) (address - self->_.startAddress) + self->addressOffset);
        if (result.has_value) {
            toReturn = (optional_funcFile_t) {
                true, (struct pair_funcFile) {
                    result.value,
                    it
                }
            };
            break;
        }
    }
    
    return toReturn;
}

static inline optional_debugInfo_t machoFile_createLocalDebugInfo(struct machoFile* self, void* address) {
    const uint64_t searchAddress = address - self->_.startAddress + self->addressOffset;
    
    struct function* closest = NULL;
    for (size_t i = 0; i < self->functions.count; ++i) {
        struct function* elem = &self->functions.content[i];

        if (closest == NULL && elem->startAddress < searchAddress) {
            closest = elem;
        } else if (closest != NULL && elem->startAddress < searchAddress && searchAddress - elem->startAddress < searchAddress - closest->startAddress) {
            closest = elem;
        }
    }
    if (closest == NULL) {
        return (optional_debugInfo_t) { .has_value = false };
    }
    return (optional_debugInfo_t) {
        true, (struct debugInfo) {
            .function                 = *closest,
            .sourceFileInfo.has_value = false
        }
    };
}

optional_debugInfo_t machoFile_getDebugInfo(struct machoFile* self, void* address) {
    for (struct objectFile* it = self->objectFiles; it != NULL; it = it->next) {
        optional_debugInfo_t result = objectFile_getDebugInfo(it, (uint64_t) (address - self->_.startAddress) + self->addressOffset);
        if (result.has_value) {
            return result;
        }
    }
    
    return machoFile_createLocalDebugInfo(self, address);
}
