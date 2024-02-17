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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <mach-o/loader.h>

#include "objectFile.h"

#include "macho_parser.h"
#include "macho_utils.h"

static inline bool objectFile_handleSegment64(struct segment_command_64* command,
                                              void*                      baseAddress,
                                              bool                       bitsSwapped,
                                              dwarf_line_callback cb, va_list args) {
    const uint32_t nsects = macho_maybeSwap(32, bitsSwapped, command->nsects);
    
    for (size_t i = 0; i < nsects; ++i) {
        struct section_64* section = (void*) command + sizeof(struct segment_command_64) + i * sizeof(struct section_64);
        
        if (strcmp("__DWARF", section->segname) == 0 &&
            strcmp("__debug_line", section->sectname) == 0) {
            if (!dwarf_parseLineProgram(baseAddress + macho_maybeSwap(32, bitsSwapped, section->offset), 
                                        cb, args,
                                        macho_maybeSwap(64, bitsSwapped, section->size))) {
                return false;
            }
        }
    }
    return true;
}

static inline bool objectFile_handleSegment(struct segment_command* command, 
                                            void*                   baseAddress,
                                            bool                    bitsSwapped,
                                            dwarf_line_callback cb, va_list args) {
    const uint32_t nsects = macho_maybeSwap(32, bitsSwapped, command->nsects);
    
    for (size_t i = 0; i < nsects; ++i) {
        struct section* section = (void*) command + sizeof(struct segment_command) + i * sizeof(struct section);
        
        if (strcmp("__DWARF", section->segname) == 0 &&
            strcmp("__debug_line", section->sectname) == 0) {
            if (!dwarf_parseLineProgram(baseAddress + macho_maybeSwap(32, bitsSwapped, section->offset), 
                                        cb, args,
                                        macho_maybeSwap(32, bitsSwapped, section->size))) {
                return false;
            }
        }
    }
    return true;
}

static inline void objectFile_addFunctionCallback(struct pair_funcFile f, va_list args) {
    objectFile_addOwnFunction(va_arg(args, void*), f.first);
}

static inline bool objectFile_parseMachOImpl64(struct objectFile* self,
                                               void*              baseAddress,
                                               bool               bitsSwapped,
                                               dwarf_line_callback cb, va_list args) {
    struct mach_header_64* header = baseAddress;
    struct load_command*   lc     = (void*) header + sizeof(struct mach_header_64);
    const  uint32_t        ncmds  = macho_maybeSwap(32, bitsSwapped, header->ncmds);
    
    for (size_t i = 0; i < ncmds; ++i) {
        bool result = true;
        switch (macho_maybeSwap(32, bitsSwapped, lc->cmd)) {
            case LC_SEGMENT_64:
                result = objectFile_handleSegment64((void*) lc, baseAddress, bitsSwapped, cb, args);
                break;
                
            case LC_SYMTAB:
                result = macho_parseSymtab((void*) lc, baseAddress, 0, bitsSwapped, true, NULL, objectFile_addFunctionCallback, self);
                break;
                
            case LC_UUID: 
                memcpy(&self->uuid, &((struct uuid_command*) ((void*) lc))->uuid, 16);
                result = true;
                break;
        }
        if (!result) {
            return false;
        }
        lc = (void*) lc + macho_maybeSwap(32, bitsSwapped, lc->cmdsize);
    }
    return true;
}

static inline bool objectFile_parseMachOImpl(struct objectFile* self,
                                             void*              baseAddress,
                                             bool               bitsSwapped,
                                             dwarf_line_callback cb, va_list args) {
    struct mach_header*  header = baseAddress;
    struct load_command* lc     = (void*) header + sizeof(struct mach_header);
    const  uint32_t      ncmds  = macho_maybeSwap(32, bitsSwapped, header->ncmds);
    
    for (size_t i = 0; i < ncmds; ++i) {
        bool result = true;
        switch (macho_maybeSwap(32, bitsSwapped, lc->cmd)) {
            case LC_SEGMENT:
                result = objectFile_handleSegment((void*) lc, baseAddress, bitsSwapped, cb, args);
                break;
                
            case LC_SYMTAB:
                result = macho_parseSymtab((void*) lc, baseAddress, 0, bitsSwapped, false, NULL, objectFile_addFunctionCallback, self);
                break;
                
            case LC_UUID:
                memcpy(&self->uuid, &((struct uuid_command*) ((void*) lc))->uuid, 16);
                result = true;
                break;
        }
        if (!result) {
            return false;
        }
        lc = (void*) lc + macho_maybeSwap(32, bitsSwapped, lc->cmdsize);
    }
    return true;
}

static inline bool objectFile_parseMachO(struct objectFile* self,
                                         void*              buffer,
                                         dwarf_line_callback cb, va_list args) {
    if (buffer == NULL) return false;
    
    struct mach_header* header = buffer;
    
    if (header->magic == MH_MAGIC    || header->magic == MH_CIGAM ||
        header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64) {
        const uint32_t fileType = macho_maybeSwap(32, header->magic == MH_CIGAM || header->magic == MH_CIGAM_64, header->filetype);
        if (fileType != MH_OBJECT && fileType != MH_DSYM) {
            return false;
        }
    }
    
    switch (header->magic) {
        case MH_MAGIC: return objectFile_parseMachOImpl(self, buffer, false, cb, args);
        case MH_CIGAM: return objectFile_parseMachOImpl(self, buffer, true,  cb, args);
            
        case MH_MAGIC_64: return objectFile_parseMachOImpl64(self, buffer, false, cb, args);
        case MH_CIGAM_64: return objectFile_parseMachOImpl64(self, buffer, true,  cb, args);
            
        case FAT_MAGIC:
        case FAT_MAGIC_64: return objectFile_parseMachO(self, macho_parseFat(buffer, false, self->name), cb, args);
            
        case FAT_CIGAM:
        case FAT_CIGAM_64: return objectFile_parseMachO(self, macho_parseFat(buffer, true, self->name), cb, args);
    }
    return false;
}

bool objectFile_parseWithBuffer(struct objectFile* self, void* buffer, dwarf_line_callback cb, ...) {
    if (self == NULL) return false;
    
    va_list args;
    va_start(args, cb);
    const bool success = objectFile_parseMachO(self, buffer, cb, args);
    va_end(args);
    return success;
}

bool objectFile_parse(struct objectFile* self, dwarf_line_callback cb, ...) {
    if (self == NULL) return false;
    
    struct stat fileStats;
    if (stat(self->name, &fileStats) != 0) {
        return false;
    }
    if (self->lastModified != 0 && fileStats.st_mtime != self->lastModified) {
        return false;
    }
    void* buffer = malloc(fileStats.st_size);
    if (buffer == NULL) {
        return false;
    }
    FILE* file = fopen(self->name, "r");
    if (file == NULL) {
        free(buffer);
        return false;
    }
    const size_t count = fread(buffer, 1, fileStats.st_size, file);
    fclose(file);
    va_list args;
    va_start(args, cb);
    const bool success = (off_t) count == fileStats.st_size && objectFile_parseMachO(self, buffer, cb, args);
    va_end(args);
    free(buffer);
    return success;
}
