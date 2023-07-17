/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023  mhahnFr
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

#include <mach-o/loader.h>
#include <mach-o/stab.h>
#include <mach-o/nlist.h>

#include "machoFileInternal.h"

#define machoFile_maybeSwap(bits, swap, value) ((swap) ? OSSwapInt##bits(value) : (value))

static inline bool machoFile_handleSegment(struct machoFile *       self,
                                           struct segment_command * segment,
                                           bool                     bitsReversed) {
    if (strcmp(segment->segname, SEG_LINKEDIT) == 0) {
        self->addressOffset = machoFile_maybeSwap(32, bitsReversed, segment->vmaddr)
                            - machoFile_maybeSwap(32, bitsReversed, segment->fileoff);
    }
    return true;
}

static inline bool machoFile_handleSegment64(struct machoFile *          self,
                                             struct segment_command_64 * segment,
                                             bool                        bitsReversed) {
    if (strcmp(segment->segname, SEG_LINKEDIT) == 0) {
        self->addressOffset = machoFile_maybeSwap(64, bitsReversed, segment->vmaddr)
                            - machoFile_maybeSwap(64, bitsReversed, segment->fileoff);
    }
    return true;
}

static inline bool machoFile_handleSymtab(struct machoFile *      self,
                                          struct symtab_command * command,
                                          void *                  baseAddress,
                                          bool                    bitsReversed) {
    char * stringBegin = baseAddress + machoFile_maybeSwap(32, bitsReversed, command->stroff);
    
    struct objectFile * current = objectFile_new();
    struct function *   currFun = NULL;
    const  uint32_t     nsyms   = machoFile_maybeSwap(32, bitsReversed, command->nsyms);
    const  uint32_t     symoff  = machoFile_maybeSwap(32, bitsReversed, command->symoff);
    
    for (size_t i = 0; i < nsyms; ++i) {
        struct nlist * entry = baseAddress + symoff + i * sizeof(struct nlist);
        switch (entry->n_type) {
            case N_BNSYM:
                if (currFun != NULL) {
                    // Function begin without begin -> invalid.
                    return false;
                }
                currFun = function_new();
                break;
                
            case N_ENSYM:
                if (currFun == NULL) {
                    // Function end without begin -> invalid.
                    return false;
                }
                objectFile_addFunction(current, currFun);
                currFun = NULL;
                break;
                
            case N_SO: {
                char * value = stringBegin + machoFile_maybeSwap(32, bitsReversed, entry->n_un.n_strx);
                if (*value == '\0') {
                    // Begin of new object file
                    machoFile_addObjectFile(self, current);
                    current = objectFile_new();
                } else {
                    if (current->directory == NULL) {
                        current->directory = value;
                    } else if (current->sourceFile == NULL) {
                        current->sourceFile = value;
                    } else {
                        // Unknown format...
                        return false;
                    }
                }
                break;
            }
                
            case N_OSO:
                current->name = stringBegin + machoFile_maybeSwap(32, bitsReversed, entry->n_un.n_strx);
                break;
                
            case N_FUN: {
                if (currFun == NULL) {
                    // Function name without begin -> invalid.
                    return false;
                }
                char * value = stringBegin + machoFile_maybeSwap(32, bitsReversed, entry->n_un.n_strx);
                if (*value != '\0') {
                    currFun->linkedName   = value;
                    currFun->startAddress = machoFile_maybeSwap(32, bitsReversed, entry->n_value);
                }
                break;
            }
        }
    }
    machoFile_addObjectFile(self, current);
    if (currFun != NULL) {
        // Function entries did not end -> invalid.
        return false;
    }
    
    return true;
}

static inline bool machoFile_handleSymtab64(struct machoFile *      self,
                                            struct symtab_command * command,
                                            void *                  baseAddress,
                                            bool                    bitsReversed) {
    char * stringBegin = baseAddress + machoFile_maybeSwap(32, bitsReversed, command->stroff);
    
    struct objectFile * current = objectFile_new();
    struct function *   currFun = NULL;
    const  uint32_t     nsyms   = machoFile_maybeSwap(32, bitsReversed, command->nsyms);
    const  uint32_t     symoff  = machoFile_maybeSwap(32, bitsReversed, command->symoff);
    
    for (size_t i = 0; i < nsyms; ++i) {
        struct nlist_64 * entry = baseAddress + symoff + i * sizeof(struct nlist_64);
        switch (entry->n_type) {
            case N_BNSYM:
                if (currFun != NULL) {
                    // Function begin without begin -> invalid.
                    return false;
                }
                currFun = function_new();
                break;
            
            case N_ENSYM:
                if (currFun == NULL) {
                    // Function end without begin -> invalid.
                    return false;
                }
                objectFile_addFunction(current, currFun);
                currFun = NULL;
                break;
            
            case N_SO: {
                char * value = stringBegin + machoFile_maybeSwap(32, bitsReversed, entry->n_un.n_strx);
                if (*value == '\0') {
                    // Begin of new object file
                    machoFile_addObjectFile(self, current);
                    current = objectFile_new();
                } else {
                    if (current->directory == NULL) {
                        current->directory = value;
                    } else if (current->sourceFile == NULL) {
                        current->sourceFile = value;
                    } else {
                        // Unknown format...
                        return false;
                    }
                }
                break;
            }
                
            case N_OSO:
                current->name = stringBegin + machoFile_maybeSwap(32, bitsReversed, entry->n_un.n_strx);
                break;
                
            case N_FUN: {
                if (currFun == NULL) {
                    // Function name without begin -> invalid.
                    return false;
                }
                char * value = stringBegin + machoFile_maybeSwap(32, bitsReversed, entry->n_un.n_strx);
                if (*value != '\0') {
                    currFun->linkedName   = value;
                    currFun->startAddress = machoFile_maybeSwap(64, bitsReversed, entry->n_value);
                }
                break;
            }
        }
    }
    machoFile_addObjectFile(self, current);
    if (currFun != NULL) {
        // Function entries did not end -> invalid.
        return false;
    }
    
    return true;
}

static inline void machoFile_addFunctionEnds(struct function * func, va_list * args) {
    struct machoFile * self = va_arg(*args, void *);

    size_t i;
    for (i = 0; i < self->functionStarts.count && self->functionStarts.content[i] != func->startAddress; ++i);

    if (i < self->functionStarts.count - 1) {
        func->endAddress = self->functionStarts.content[i + 1];
    } else {
        // TODO: Was dann?
    }
}

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

bool machoFile_parseFile(struct machoFile * self, void * baseAddress) {
    if (strcmp(self->_.fileName, "/usr/lib/dyld") == 0) {
        // We cannot parse Darwins dynamic linker at the moment.
        return false;
    }
    
    struct mach_header * header = baseAddress;
    switch (header->magic) {
        case MH_MAGIC: return machoFile_parseFileImpl(self, baseAddress, false);
        case MH_CIGAM: return machoFile_parseFileImpl(self, baseAddress, true);
            
        case MH_MAGIC_64: return machoFile_parseFileImpl64(self, baseAddress, false);
        case MH_CIGAM_64: return machoFile_parseFileImpl64(self, baseAddress, true);
    }
    return false;
}

void machoFile_addObjectFile(struct machoFile *  self,
                             struct objectFile * file) {
    file->next        = self->objectFiles;
    self->objectFiles = file;
}

struct function * machoFile_findClosestFunction(struct machoFile * self, void * startAddress, void * address,
                                                struct objectFile ** filePtr) {
    address += self->addressOffset;
    
    int64_t distance = INT64_MAX;
    struct function * func = NULL;
    struct objectFile * file = NULL;
    for (struct objectFile * it = self->objectFiles; it != NULL; it = it->next) {
        struct function * tmp = NULL;
        int64_t dist = objectFile_findClosestFunction(it, (uint64_t) (address - startAddress), &tmp);
        if (dist < distance && dist >= 0) {
            distance = dist;
            func = tmp;
            file = it;
        }
    }
    *filePtr = file;
    return distance < INT64_MAX ? func : NULL;
}
