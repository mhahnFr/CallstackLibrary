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

static inline bool machoFile_handleSegment(struct machoFile *       self,
                                           struct segment_command * segment,
                                           bool                     bitsReversed) {
    // TODO: Handle bit reversion
    (void) bitsReversed;
    
    if (strcmp(segment->segname, SEG_LINKEDIT) == 0) {
        self->addressOffset = segment->vmaddr - segment->fileoff;
    }
    return true;
}

static inline bool machoFile_handleSegment64(struct machoFile *          self,
                                             struct segment_command_64 * segment,
                                             bool                        bitsReversed) {
    // TODO: Handle bit reversion
    (void) bitsReversed;
    
    if (strcmp(segment->segname, SEG_LINKEDIT) == 0) {
        self->addressOffset = segment->vmaddr - segment->fileoff;
    }
    return true;
}

static inline bool machoFile_handleSymtab(struct machoFile *      self,
                                          struct symtab_command * command,
                                          void *                  baseAddress,
                                          bool                    bitsReversed) {
    // TODO: Handle bit reversion
    (void) bitsReversed;
    
    char * stringBegin = baseAddress + command->stroff;
    
    struct objectFile * current = objectFile_new();
    struct function *   currFun = NULL;
    for (size_t i = 0; i < command->nsyms; ++i) {
        struct nlist * entry = baseAddress + command->symoff + i * sizeof(struct nlist);
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
                char * value = stringBegin + entry->n_un.n_strx;
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
                current->name = stringBegin + entry->n_un.n_strx;
                break;
                
            case N_FUN: {
                if (currFun == NULL) {
                    // Function name without begin -> invalid.
                    return false;
                }
                char * value = stringBegin + entry->n_un.n_strx;
                if (*value != '\0') {
                    currFun->linkedName   = value;
                    currFun->startAddress = entry->n_value;
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
    // TODO: Handle bit reversion
    (void) bitsReversed;
    
    char * stringBegin = baseAddress + command->stroff;
    
    struct objectFile * current = objectFile_new();
    struct function *   currFun = NULL;
    for (size_t i = 0; i < command->nsyms; ++i) {
        struct nlist_64 * entry = baseAddress + command->symoff + i * sizeof(struct nlist_64);
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
                char * value = stringBegin + entry->n_un.n_strx;
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
                current->name = stringBegin + entry->n_un.n_strx;
                break;
                
            case N_FUN: {
                if (currFun == NULL) {
                    // Function name without begin -> invalid.
                    return false;
                }
                char * value = stringBegin + entry->n_un.n_strx;
                if (*value != '\0') {
                    currFun->linkedName   = value;
                    currFun->startAddress = entry->n_value;
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

static inline bool machoFile_parseFileImpl(struct machoFile * self,
                                           void *             baseAddress,
                                           bool               bitsReversed) {
    // TODO: Handle bit reversion
    struct mach_header * header = baseAddress;
    
    struct load_command * lc = (void *) header + sizeof(struct mach_header);
    for (size_t i = 0; i < header->ncmds; ++i) {
        bool result = true;
        switch (lc->cmd) {
            case LC_SEGMENT: result = machoFile_handleSegment(self, (void *) lc, bitsReversed);        break;
            case LC_SYMTAB:  result = machoFile_handleSymtab(self, (void *) lc, header, bitsReversed); break;
        }
        if (!result) {
            return false;
        }
        lc = (void *) lc + lc->cmdsize;
    }
    return true;
}

static inline bool machoFile_parseFileImpl64(struct machoFile * self,
                                             void *             baseAddress,
                                             bool               bitsReversed) {
    // TODO: Handle bit reversion
    struct mach_header_64 * header = baseAddress;
    
    struct load_command * lc = (void *) header + sizeof(struct mach_header_64);
    for (size_t i = 0; i < header->ncmds; ++i) {
        bool result = true;
        switch (lc->cmd) {
            case LC_SEGMENT_64: result = machoFile_handleSegment64(self, (void *) lc, bitsReversed);        break;
            case LC_SYMTAB:     result = machoFile_handleSymtab64(self, (void *) lc, header, bitsReversed); break;
        }
        if (!result) {
            return false;
        }
        lc = (void *) lc + lc->cmdsize;
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
