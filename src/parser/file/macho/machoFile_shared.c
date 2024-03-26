/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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

#include <mach-o/fat.h>
#include <mach-o/loader.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "cache.h"
#include "machoFile.h"
#include "macho_parser.h"
#include "macho_utils.h"

#include "../utils.h"

#include "../../callstack_parser.h"

/**
 * Loads and parses the Mach-O file represented by the given Mach-O file abstraction object.
 *
 * @param self the Mach-O file abstraction object
 * @return whether the file was parsed successfully
 */
static inline bool machoFile_loadFile(struct machoFile* self) {
    return self->inMemory ? machoFile_parseFile(self, self->_.startAddress)
                          : loader_loadFileAndExecute(self->_.fileName, (loader_parser) machoFile_parseFile, self);
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
    self->priv             = NULL;
    self->inMemory         = macho_cache_isLoaded(self);
    
    self->dSYMFile.triedParsing = false;
    self->dSYMFile.file         = NULL;
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

struct objectFile* machoFile_getDSYMBundle(struct machoFile* self) {
    if (!self->dSYMFile.triedParsing) {
        self->dSYMFile.file = machoFile_findDSYMBundle(self);
        self->dSYMFile.triedParsing = true;
    }
    return self->dSYMFile.file;
}

void machoFile_clearCaches(void) {
    macho_cache_destroy();
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
            frame->sourceFile = binaryFile_toAbsolutePath((char*) result.value.sourceFileInfo.value.sourceFile);
            frame->sourceFileRelative = binaryFile_toRelativePath((char*) result.value.sourceFileInfo.value.sourceFile);
            frame->sourceLine = result.value.sourceFileInfo.value.line;
            if (result.value.sourceFileInfo.value.column > 0) {
                frame->sourceLineColumn = (optional_ulong_t) { true, result.value.sourceFileInfo.value.column };
            }
            frame->function = name;
        } else {
            char* toReturn = NULL;
            asprintf(&toReturn, "%s + %td",
                     name,
                     (ptrdiff_t) (address - self->_.startAddress 
                                  + (self->inMemory ? self->text_vmaddr : self->addressOffset)
                                  - result.value.function.startAddress));
            free(name);
            frame->function = toReturn;
        }
        return true;
    }
    return false;
}

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
 * @param bitsReversed whether to swap the numbers' endianess
 * @return whether the handling was successfull
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
static inline void machoFile_addFunctionImpl(struct pair_funcFile function, va_list args) {
    struct machoFile* self = va_arg(args, void*);
    
    machoFile_addFunction(self, function);
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
                result = macho_parseSymtab((void*) lc, baseAddress, 
                                           self->inMemory ? (self->linkedit_vmaddr - self->text_vmaddr) - self->linkedit_fileoff : 0,
                                           bitsReversed, false, NULL, machoFile_addFunctionImpl, self);
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
                result = macho_parseSymtab((void*) lc, baseAddress, 
                                           self->inMemory ? (self->linkedit_vmaddr - self->text_vmaddr) - self->linkedit_fileoff : 0,
                                           bitsReversed, true, NULL, machoFile_addFunctionImpl, self);
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

bool machoFile_parseFile(struct machoFile * self, void * baseAddress) {
    if (baseAddress == NULL) return false;
    
    struct mach_header * header = baseAddress;
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
