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

#include "macho_utils.h"

static inline bool objectFile_handleSegment64(struct segment_command_64* command, void* baseAddress, bool bitsSwapped, dwarf_line_callback cb) {
    const uint32_t nsects = macho_maybeSwap(32, bitsSwapped, command->nsects);
    
    for (size_t i = 0; i < nsects; ++i) {
        struct section_64* section = (void*) command + sizeof(struct segment_command_64) + i * sizeof(struct section_64);
        
        if (strcmp("__DWARF", section->segname) == 0 &&
            strcmp("__debug_line", section->sectname) == 0) {
            if (!dwarf_parseLineProgram(baseAddress + macho_maybeSwap(32, bitsSwapped, section->offset), cb)) {
                return false;
            }
        }
    }
    return true;
}

static inline bool objectFile_handleSegment(struct segment_command* command, void* baseAddress, bool bitsSwapped, dwarf_line_callback cb) {
    const uint32_t nsects = macho_maybeSwap(32, bitsSwapped, command->nsects);
    
    for (size_t i = 0; i < nsects; ++i) {
        struct section* section = (void*) command + sizeof(struct segment_command) + i * sizeof(struct section);
        
        if (strcmp("__DWARF", section->segname) == 0 &&
            strcmp("__debug_line", section->sectname) == 0) {
            if (!dwarf_parseLineProgram(baseAddress + macho_maybeSwap(32, bitsSwapped, section->offset), cb)) {
                return false;
            }
        }
    }
    return true;
}

static inline bool objectFile_parseMachOImpl64(void* baseAddress, bool bitsSwapped, dwarf_line_callback cb) {
    struct mach_header_64* header = baseAddress;
    struct load_command*   lc     = (void*) header + sizeof(struct mach_header_64);
    const  uint32_t        ncmds  = macho_maybeSwap(32, bitsSwapped, header->ncmds);
    
    for (size_t i = 0; i < ncmds; ++i) {
        bool result = true;
        switch (macho_maybeSwap(32, bitsSwapped, lc->cmd)) {
            case LC_SEGMENT_64: result = objectFile_handleSegment64((void*) lc, baseAddress, bitsSwapped, cb); break;
        }
        if (!result) {
            return false;
        }
        lc = (void*) lc + macho_maybeSwap(32, bitsSwapped, lc->cmdsize);
    }
    return true;
}

static inline bool objectFile_parseMachOImpl(void* baseAddress, bool bitsSwapped, dwarf_line_callback cb) {
    struct mach_header*  header = baseAddress;
    struct load_command* lc     = (void*) header + sizeof(struct mach_header);
    const  uint32_t      ncmds  = macho_maybeSwap(32, bitsSwapped, header->ncmds);
    
    for (size_t i = 0; i < ncmds; ++i) {
        bool result = true;
        switch (macho_maybeSwap(32, bitsSwapped, lc->cmd)) {
            case LC_SEGMENT: result = objectFile_handleSegment((void*) lc, baseAddress, bitsSwapped, cb); break;
        }
        if (!result) {
            return false;
        }
        lc = (void*) lc + macho_maybeSwap(32, bitsSwapped, lc->cmdsize);
    }
    return true;
}

static inline bool objectFile_parseMachO(void* buffer, dwarf_line_callback cb) {
    struct mach_header* header = buffer;
    switch (header->magic) {
        case MH_MAGIC: return objectFile_parseMachOImpl(buffer, false, cb);
        case MH_CIGAM: return objectFile_parseMachOImpl(buffer, true,  cb);
            
        case MH_MAGIC_64: return objectFile_parseMachOImpl64(buffer, false, cb);
        case MH_CIGAM_64: return objectFile_parseMachOImpl64(buffer, true,  cb);
            
        // We do not parse fat Mach-O object files for now.
        // If this becomes necessary, refer to the implementation
        // of machoFile_parseFat.
        //                                          - mhahnFr
    }
    return false;
}

bool objectFile_parse(struct objectFile* self, dwarf_line_callback cb) {
    if (self == NULL) return false;
    
    struct stat fileStats;
    if (stat(self->name, &fileStats) != 0) {
        return false;
    }
    if (fileStats.st_mtime > self->lastModified) {
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
    const bool success = (off_t) count == fileStats.st_size && objectFile_parseMachO(buffer, cb);
    free(buffer);
    return success;
}
