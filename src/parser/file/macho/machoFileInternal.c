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
#include <mach-o/nlist.h>
#include <mach-o/stab.h>

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

#define machoFile_maybeSwap(bits, swap, value) ((swap) ? OSSwapInt##bits(value) : (value))

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
        self->addressOffset = machoFile_maybeSwap(32, bitsReversed, segment->vmaddr)
                            + machoFile_maybeSwap(32, bitsReversed, segment->vmsize);
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
        self->addressOffset = machoFile_maybeSwap(64, bitsReversed, segment->vmaddr)
                            + machoFile_maybeSwap(64, bitsReversed, segment->vmsize);
    }
    return true;
}

/**
 * Handles the given symtab command.
 *
 * @param self the Mach-O file abstraction object
 * @param command the symtab command to handle
 * @param baseAddress the baseAddress of the Mach-O file
 * @param bitsReversed whether to swap the numbers' endianess
 * @return whether the handling was successfull
 */
static inline bool machoFile_handleSymtab(struct machoFile *      self,
                                          struct symtab_command * command,
                                          void *                  baseAddress,
                                          bool                    bitsReversed) {
    char * stringBegin = baseAddress + machoFile_maybeSwap(32, bitsReversed, command->stroff);
    
    struct objectFile *      current = objectFile_new();
    struct optional_function currFun = { .has_value = false };
    const  uint32_t          nsyms   = machoFile_maybeSwap(32, bitsReversed, command->nsyms);
    const  uint32_t          symoff  = machoFile_maybeSwap(32, bitsReversed, command->symoff);
    
    for (size_t i = 0; i < nsyms; ++i) {
        struct nlist * entry = baseAddress + symoff + i * sizeof(struct nlist);
        switch (entry->n_type) {
            case N_BNSYM:
                if (currFun.has_value) {
                    // Function begin without end -> invalid.
                    function_destroy(&currFun.value);
                    objectFile_delete(current);
                    return false;
                }
                function_create(&currFun.value);
                currFun.has_value = true;
                break;
                
            case N_ENSYM:
                if (!currFun.has_value) {
                    // Function end without begin -> invalid.
                    objectFile_delete(current);
                    return false;
                }
                objectFile_addFunction(current, currFun.value);
                currFun.has_value = false;
                break;
                
            case N_SO: {
                char * value = stringBegin + machoFile_maybeSwap(32, bitsReversed, entry->n_un.n_strx);
                if (*value == '\0') {
                    // Begin of new object file
                    machoFile_addObjectFile(self, current);
                    current = objectFile_new();
                } else {
                    if (current->directory == NULL) {
                        current->directory = strdup(value);
                    } else if (current->sourceFile == NULL) {
                        current->sourceFile = strdup(value);
                    } else {
                        // Unknown format...
                        objectFile_delete(current);
                        if (currFun.has_value) {
                            function_destroy(&currFun.value);
                        }
                        return false;
                    }
                }
                break;
            }
                
            case N_OSO:
                current->name = strdup(stringBegin + machoFile_maybeSwap(32, bitsReversed, entry->n_un.n_strx));
                break;
                
            case N_FUN: {
                if (!currFun.has_value) {
                    // Function name without begin -> invalid.
                    objectFile_delete(current);
                    return false;
                }
                char * value = stringBegin + machoFile_maybeSwap(32, bitsReversed, entry->n_un.n_strx);
                if (*value != '\0') {
                    currFun.value.linkedName   = strdup(value);
                    currFun.value.startAddress = machoFile_maybeSwap(32, bitsReversed, entry->n_value);
                }
                break;
            }
        }
    }
    machoFile_addObjectFile(self, current);
    if (currFun.has_value) {
        // Function entries did not end -> invalid.
        function_destroy(&currFun.value);
        return false;
    }
    
    return true;
}

/**
 * Handles the given 64 bit symtab command.
 *
 * @param self the Mach-O file abstraction object
 * @param command the 64 bit symtab command to handle
 * @param baseAddress the base address of the Mach-O file
 * @param bitsReversed whether to swap the numbers' endianess
 * @return whether the handling was successfull
 */
static inline bool machoFile_handleSymtab64(struct machoFile *      self,
                                            struct symtab_command * command,
                                            void *                  baseAddress,
                                            bool                    bitsReversed) {
    char * stringBegin = baseAddress + machoFile_maybeSwap(32, bitsReversed, command->stroff);
    
    struct objectFile *      current = objectFile_new();
    struct optional_function currFun = { .has_value = false };
    const  uint32_t          nsyms   = machoFile_maybeSwap(32, bitsReversed, command->nsyms);
    const  uint32_t          symoff  = machoFile_maybeSwap(32, bitsReversed, command->symoff);
    
    for (size_t i = 0; i < nsyms; ++i) {
        struct nlist_64 * entry = baseAddress + symoff + i * sizeof(struct nlist_64);
        switch (entry->n_type) {
            case N_BNSYM:
                if (currFun.has_value) {
                    // Function begin without end -> invalid.
                    function_destroy(&currFun.value);
                    objectFile_delete(current);
                    return false;
                }
                function_create(&currFun.value);
                currFun.has_value = true;
                break;
            
            case N_ENSYM:
                if (!currFun.has_value) {
                    // Function end without begin -> invalid.
                    objectFile_delete(current);
                    return false;
                }
                objectFile_addFunction(current, currFun.value);
                currFun.has_value = false;
                break;
            
            case N_SO: {
                char * value = stringBegin + machoFile_maybeSwap(32, bitsReversed, entry->n_un.n_strx);
                if (*value == '\0') {
                    // Begin of new object file
                    machoFile_addObjectFile(self, current);
                    current = objectFile_new();
                } else {
                    if (current->directory == NULL) {
                        current->directory = strdup(value);
                    } else if (current->sourceFile == NULL) {
                        current->sourceFile = strdup(value);
                    } else {
                        // Unknown format...
                        objectFile_delete(current);
                        if (currFun.has_value) {
                            function_destroy(&currFun.value);
                        }
                        return false;
                    }
                }
                break;
            }
                
            case N_OSO:
                current->name = strdup(stringBegin + machoFile_maybeSwap(32, bitsReversed, entry->n_un.n_strx));
                break;
                
            case N_FUN: {
                if (!currFun.has_value) {
                    // Function name without begin -> invalid.
                    objectFile_delete(current);
                    return false;
                }
                char * value = stringBegin + machoFile_maybeSwap(32, bitsReversed, entry->n_un.n_strx);
                if (*value != '\0') {
                    currFun.value.linkedName   = strdup(value);
                    currFun.value.startAddress = machoFile_maybeSwap(64, bitsReversed, entry->n_value);
                }
                break;
            }
        }
    }
    machoFile_addObjectFile(self, current);
    if (currFun.has_value) {
        // Function entries did not end -> invalid.
        function_destroy(&currFun.value);
        return false;
    }
    
    return true;
}

/**
 * Adds the end address of the given function if it was found.
 *
 * @param func the function whose end address to set
 * @param args the argument list - should contain a Mach-O file abstraction object as first argument
 */
static inline void machoFile_addFunctionEnds(struct function * func, va_list args) {
    struct machoFile * self = va_arg(args, void *);

    size_t i;
    for (i = 0; i < self->functionStarts.count && self->functionStarts.content[i] != func->startAddress; ++i);

    if (i < self->functionStarts.count - 1) {
        func->endAddress = self->functionStarts.content[i + 1];
    } else {
        /*
         * Should not happen. If it does, the function at that
         * address will be ignored when searching for the function
         * inside which a given address is in.
         *                                              - mhahnFr
         */
    }
}

/**
 * Handles the function starts command. These are inside a linkedit data command.
 *
 * @param self the Mach-O file abstraction object
 * @param command the linkedit data command to handle
 * @param baseAddress the base address of the Mach-O file
 * @param bitsReversed whether to swap the numbers' endianess
 * @return whether the handling was successfull
 */
static inline bool machoFile_handleFunctionStarts(struct machoFile *             self,
                                                  struct linkedit_data_command * command,
                                                  void *                         baseAddress,
                                                  bool                           bitsReversed) {
    uint32_t offset = machoFile_maybeSwap(32, bitsReversed, command->dataoff);
    uint32_t size   = machoFile_maybeSwap(32, bitsReversed, command->datasize);
    
    uint8_t * bytes    = baseAddress + offset;
    uint64_t  funcAddr = self->addressOffset;
    for (size_t i = 0; i < size;) {
        uint64_t result = 0,
                 shift  = 0;
        bool more = true;
        do {
            uint8_t b = bytes[i++];
            result |= (b & 0x7f) << shift;
            shift += 7;
            if (b < 0x80) {
                funcAddr += result;
                vector_uint64_t_push_back(&self->functionStarts, funcAddr);
                more = false;
            }
        } while (more);
    }
    
    for (struct objectFile * it = self->objectFiles; it != NULL; it = it->next) {
        objectFile_functionsForEach(it, &machoFile_addFunctionEnds, self);
    }
    
    return true;
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
    const  uint32_t       ncmds  = machoFile_maybeSwap(32, bitsReversed, header->ncmds);
    
    for (size_t i = 0; i < ncmds; ++i) {
        bool result = true;
        switch (machoFile_maybeSwap(32, bitsReversed, lc->cmd)) {
            case LC_SEGMENT:         result = machoFile_handleSegment(self, (void *) lc, bitsReversed);                 break;
            case LC_SYMTAB:          result = machoFile_handleSymtab(self, (void *) lc, header, bitsReversed);          break;
            case LC_FUNCTION_STARTS: result = machoFile_handleFunctionStarts(self, (void *) lc, header,  bitsReversed); break;
        }
        if (!result) {
            return false;
        }
        lc = (void *) lc + machoFile_maybeSwap(32, bitsReversed, lc->cmdsize);
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
    const  uint32_t         ncmds  = machoFile_maybeSwap(32, bitsReversed, header->ncmds);
    
    for (size_t i = 0; i < ncmds; ++i) {
        bool result = true;
        switch (machoFile_maybeSwap(32, bitsReversed, lc->cmd)) {
            case LC_SEGMENT_64:      result = machoFile_handleSegment64(self, (void *) lc, bitsReversed);              break;
            case LC_SYMTAB:          result = machoFile_handleSymtab64(self, (void *) lc, header, bitsReversed);       break;
            case LC_FUNCTION_STARTS: result = machoFile_handleFunctionStarts(self, (void *) lc, header, bitsReversed); break;
        }
        if (!result) {
            return false;
        }
        lc = (void *) lc + machoFile_maybeSwap(32, bitsReversed, lc->cmdsize);
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
            const struct fat_arch_64 * best = NXFindBestFatArch_64(machoFile_maybeSwap(32, bitsReversed, cputype),
                                                                   machoFile_maybeSwap(32, bitsReversed, cpusubtype),
                                                                   (void *) fatHeader + sizeof(struct fat_header),
                                                                   machoFile_maybeSwap(32, bitsReversed, fatHeader->nfat_arch));
            if (best == NULL) {
                return false;
            }
            offset = machoFile_maybeSwap(64, bitsReversed, best->offset);
            break;
        }
            
        case FAT_MAGIC:
        case FAT_CIGAM: {
            const struct fat_arch * best = NXFindBestFatArch(machoFile_maybeSwap(32, bitsReversed, cputype),
                                                             machoFile_maybeSwap(32, bitsReversed, cpusubtype),
                                                             (void *) fatHeader + sizeof(struct fat_header),
                                                             machoFile_maybeSwap(32, bitsReversed, fatHeader->nfat_arch));
            if (best == NULL) {
                return false;
            }
            offset = machoFile_maybeSwap(32, bitsReversed, best->offset);
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

struct optional_funcFile machoFile_findFunction(struct machoFile * self, void * startAddress, void * address) {
    struct optional_funcFile toReturn = { .has_value = false };
    
    for (struct objectFile * it = self->objectFiles; it != NULL; it = it->next) {
        struct optional_function result = objectFile_findFunction(it, (uint64_t) (address - startAddress) + self->addressOffset);
        if (result.has_value) {
            toReturn.has_value    = true;
            toReturn.value.first  = result.value;
            toReturn.value.second = it;
            break;
        }
    }
    
    return toReturn;
}
