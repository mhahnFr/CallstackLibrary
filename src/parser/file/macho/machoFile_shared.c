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

#include <mach-o/dyld.h>
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "machoFile.h"
#include "macho_parser.h"
#include "macho_utils.h"
#include "vector_boolString.h"

#include "../../callstack_parser.h"

static struct vector_boolString loadedFiles = { 0, 0, NULL };

/**
 * Caches the represented file from disk and parses it.
 *
 * @param self the Mach-O file object
 * @return whether the file was successfully read and parsed
 */
static inline bool machoFile_readAndParseFile(struct machoFile* self) {
    if (self->_.fileName == NULL) return false;

    struct stat fileStats;
    if (stat(self->_.fileName, &fileStats) != 0) {
        return false;
    }
    void * buffer = malloc(fileStats.st_size);
    if (buffer == NULL) {
        return false;
    }
    FILE * file = fopen(self->_.fileName, "r");
    const size_t count = fread(buffer, 1, fileStats.st_size, file);
    fclose(file);
    const bool toReturn = (off_t) count == fileStats.st_size && machoFile_parseFile(self, buffer);
    free(buffer);
    return toReturn;
}

static inline bool machoFile_loadFile(struct machoFile* self) {
    return self->inMemory ? machoFile_parseFile(self, self->_.startAddress)
                          : machoFile_readAndParseFile(self);
}

static inline void machoFile_loadLoadedFiles(void) {
    const uint32_t size = _dyld_image_count();
    
    for (uint32_t i = 0; i < size; ++i) {
        vector_boolString_push_back(&loadedFiles, (pair_boolString_t) { true, _dyld_get_image_name(i) });
    }
}

bool machoFile_isLoaded(struct machoFile* self) {
    if (loadedFiles.count == 0) {
        machoFile_loadLoadedFiles();
    }
    
    vector_iterate(pair_boolString_t, &loadedFiles, {
        if (strcmp(element->second, self->_.fileName) == 0) {
            return element->first;
        }
    })
    return false;
}

void machoFile_clearCaches(void) {
    vector_boolString_destroy(&loadedFiles);
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

static inline void machoFile_addObjectFileImpl(struct objectFile* objectFile, va_list args) {
    struct machoFile* self = va_arg(args, void*);
    
    machoFile_addObjectFile(self, objectFile);
}

static inline void machoFile_addFunctionImpl(struct function function, va_list args) {
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
                                           bitsReversed, false, machoFile_addObjectFileImpl, machoFile_addFunctionImpl, self);
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
                                           bitsReversed, true, machoFile_addObjectFileImpl, machoFile_addFunctionImpl, self);
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
