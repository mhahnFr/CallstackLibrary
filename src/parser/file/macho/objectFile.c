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
#include <sys/stat.h>

#include <mach-o/loader.h>

#include <macho/fat_handler.h>
#include <macho/macho_utils.h>

#include "objectFile.h"
#include "macho_parser.h"

#include "../binaryFile.h"
#include "../bounds.h"

#include "../dwarf/dwarf_parser.h"

struct objectFile* objectFile_new(void) {
    struct objectFile* self = malloc(sizeof(struct objectFile));
    if (self == NULL) {
        return NULL;
    }
    objectFile_create(self);
    return self;
}

/**
 * The callback function for adding a deducted DWARF line information.
 *
 * @param info the DWARF line info entry
 * @param args the variadic arguments - should include as the first argument the object file object
 */
static inline void objectFile_dwarfLineCallback(struct dwarf_lineInfo info, void* args) {
    struct objectFile* self = args;
    
    vector_dwarfLineInfo_push_back(&self->lineInfos, info);
}

/**
 * @brief Returns how the the two given DWARF line infos compare.
 *
 * Sorted descendingly.
 *
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return `0` if the two infos compare equal, a value smaller or bigger than `0` according to the sort order
 */
static inline int objectFile_dwarfLineInfoSortCompare(const void* lhs, const void* rhs) {
    const struct dwarf_lineInfo* a = lhs;
    const struct dwarf_lineInfo* b = rhs;

    if (a->address < b->address) return +1;
    if (a->address > b->address) return -1;

    return 0;
}

/**
 * @brief Returns how the two given functions compare.
 *
 * Sorted ascendingly.
 *
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return the difference between the two given functions
 */
static inline int objectFile_functionCompare(const void* lhs, const void* rhs) {
    const struct function* a = lhs;
    const struct function* b = rhs;

    return strcmp(a->linkedName, b->linkedName);
}

/**
 * Finds and returns the function with the given name deducted from the represented object file.
 *
 * @param self the object file object
 * @param name the name of the desired function
 * @return the optionally found function
 */
static inline optional_function_t objectFile_findOwnFunction(struct objectFile* self, const char* name) {
    optional_function_t toReturn = { .has_value = false };
    
    struct function searched = (struct function) { .linkedName = (char*) name };
    struct function* it = bsearch(&searched, self->ownFunctions.content, self->ownFunctions.count, sizeof(struct function), objectFile_functionCompare);
    if (it != NULL) {
        toReturn = (struct optional_function) { true, *it };
    }
    
    return toReturn;
}

/**
 * @brief Returns the full source file name found in the referencing Mach-O executable / library file.
 *
 * @note Do not free the returned string.
 *
 * @param self the object file object
 * @return the full source file name or `NULL` if the allocation failed
 */
static inline const char* objectFile_getSourceFileName(struct objectFile* self) {
    if (self->mainSourceFileCache != NULL) return self->mainSourceFileCache;
    if (self->directory == NULL || self->sourceFile == NULL) return "<< Unknown >>";
    
    const size_t size = strlen(self->directory) + strlen(self->sourceFile) + 1;
    char* toReturn = malloc(size);
    if (toReturn == NULL) {
        return self->sourceFile;
    }
    strlcpy(toReturn, self->directory, size);
    strlcat(toReturn, self->sourceFile, size);
    toReturn[size - 1] = '\0';
    return self->mainSourceFileCache = toReturn;
}

optional_debugInfo_t objectFile_getDebugInfo(struct objectFile* self, uint64_t address, struct function function) {
    optional_debugInfo_t toReturn = { .has_value = false };
    
    if (!self->parsed) {
        if (!(self->parsed = objectFile_parse(self))) {
            return toReturn;
        }
    }
    uint64_t lineAddress;
    if (self->isDsymBundle) {
        lineAddress = address;
    } else {
        optional_function_t ownFunction = objectFile_findOwnFunction(self, function.linkedName);
        if (!ownFunction.has_value) {
            return toReturn;
        }
        lineAddress = ownFunction.value.startAddress + address - function.startAddress;
    }

    struct dwarf_lineInfo tmp = (struct dwarf_lineInfo) { .address = lineAddress };
    const struct dwarf_lineInfo* closest = upper_bound(&tmp, 
                                                       self->lineInfos.content,
                                                       self->lineInfos.count,
                                                       sizeof(struct dwarf_lineInfo),
                                                       objectFile_dwarfLineInfoSortCompare);
    if (closest == NULL) {
        return toReturn;
    }
    return (optional_debugInfo_t) {
        true, (struct debugInfo) {
            function, (optional_sourceFileInfo_t) {
                true, (struct sourceFileInfo) {
                    closest->line,
                    closest->column,
                    closest->sourceFile.fileName == NULL ? objectFile_getSourceFileName(self) : closest->sourceFile.fileName,
                    binaryFile_isOutdated(closest->sourceFile)
                }
            }
        }
    };
}

uint8_t* objectFile_getUUID(struct objectFile* self) {
    if (!self->parsed) {
        self->parsed = objectFile_parse(self);
    }
    return self->uuid;
}

/**
 * Handles the given section.
 *
 * @param self the object file object
 * @param section the translated section to handle
 * @param segname the segment name the section is found in
 * @param sectname the section's name
 */
static inline void objectFile_handleSection(struct objectFile* self,
                                            struct lcs_section section,
                                            const char*        segname,
                                            const char*        sectname) {
    if (strcmp("__DWARF", segname) == 0) {
        if (strcmp("__debug_line", sectname) == 0) {
            self->debugLine = section;
        } else if (strncmp("__debug_line_str", sectname, 16) == 0) {
            self->debugLineStr = section;
        } else if (strcmp("__debug_str", sectname) == 0) {
            self->debugStr = section;
        } else if (strcmp("__debug_info", sectname) == 0) {
            self->debugInfo = section;
        } else if (strcmp("__debug_abbrev", sectname) == 0) {
            self->debugAbbrev = section;
        } else if (strncmp("__debug_str_offsets", sectname, 16) == 0) {
            self->debugStrOffsets = section;
        }
    }
}

/**
 * @brief Handles the given 64 bit segment command.
 *
 * If the given segment contains the DWARF debug line information, it is parsed using the given
 * DWARF line callback and the given additional arguments.
 *
 * @param self the object file object
 * @param command the command to process
 * @param baseAddress the beginning of the Mach-O file
 * @param bytesSwapped whether the bytes need to be swapped to match the host byte order
 * @return whether the handling (and optional DWARF parsing) was successful
 */
static inline bool objectFile_handleSegment64(struct objectFile*         self,
                                              struct segment_command_64* command,
                                              void*                      baseAddress,
                                              bool                       bytesSwapped) {
    const uint32_t nsects = macho_maybeSwap(32, bytesSwapped, command->nsects);

    for (size_t i = 0; i < nsects; ++i) {
        struct section_64* section = (void*) command + sizeof(struct segment_command_64) + i * sizeof(struct section_64);
        objectFile_handleSection(self, (struct lcs_section) {
            baseAddress + macho_maybeSwap(32, bytesSwapped, section->offset),
            macho_maybeSwap(64, bytesSwapped, section->size)
        }, section->segname, section->sectname);
    }
    return true;
}

/**
 * @brief Handles the given 32 bit segment command.
 *
 * If the given segment contains the DWARF debug line information, it is parsed using the given
 * DWARF line callback and the given additional arguments.
 *
 * @param self the object file object
 * @param command the command to process
 * @param baseAddress the beginning of the Mach-O file
 * @param bytesSwapped whether the bytes need to be swapped to match the host byte order
 * @return whether the handling (and optional DWARF parsing) was successful
 */
static inline bool objectFile_handleSegment(struct objectFile*      self,
                                            struct segment_command* command,
                                            void*                   baseAddress,
                                            bool                    bytesSwapped) {
    const uint32_t nsects = macho_maybeSwap(32, bytesSwapped, command->nsects);

    for (size_t i = 0; i < nsects; ++i) {
        struct section* section = (void*) command + sizeof(struct segment_command) + i * sizeof(struct section);
        objectFile_handleSection(self, (struct lcs_section) {
            baseAddress + macho_maybeSwap(32, bytesSwapped, section->offset),
            macho_maybeSwap(32, bytesSwapped, section->size)
        }, section->segname, section->sectname);
    }
    return true;
}

/**
 * The callback adding the function / object file pair to the object file object passed via the variadic argument list.
 *
 * @param f the function / object file pair
 * @param args the arguments - should contain as first argument the object file object ot add the pair to
 */
static inline void objectFile_addFunctionCallback(struct pair_funcFile f, va_list args) {
    struct objectFile* self = va_arg(args, void*);

    vector_function_push_back(&self->ownFunctions, f.first);
}

/**
 * Parses the Mach-O file into the given object file object. (64 bit version).
 *
 * @param self the object file object
 * @param baseAddress the beginning address of the Mach-O file to be parsed
 * @param bytesSwapped whether the bytes need to be swapped to match the host byte order
 * @return whether the parsing was successful
 */
static inline bool objectFile_parseMachOImpl64(struct objectFile* self,
                                               void*              baseAddress,
                                               bool               bytesSwapped) {
    struct mach_header_64* header = baseAddress;
    struct load_command*   lc     = (void*) header + sizeof(struct mach_header_64);
    const  uint32_t        ncmds  = macho_maybeSwap(32, bytesSwapped, header->ncmds);

    for (size_t i = 0; i < ncmds; ++i) {
        bool result = true;
        switch (macho_maybeSwap(32, bytesSwapped, lc->cmd)) {
            case LC_SEGMENT_64:
                result = objectFile_handleSegment64(self, (void*) lc, baseAddress, bytesSwapped);
                break;

            case LC_SYMTAB:
                result = macho_parseSymtab((void*) lc, baseAddress, 0, bytesSwapped, true, NULL, objectFile_addFunctionCallback, self);
                break;

            case LC_UUID:
                memcpy(&self->uuid, &((struct uuid_command*) ((void*) lc))->uuid, 16);
                result = true;
                break;
        }
        if (!result) {
            return false;
        }
        lc = (void*) lc + macho_maybeSwap(32, bytesSwapped, lc->cmdsize);
    }
    return true;
}

/**
 * Parses the Mach-O file into the given object file object.
 *
 * @param self the object file object
 * @param baseAddress the beginning address of the Mach-O file to be parsed
 * @param bytesSwapped whether the bytes need to be swapped to match the host byte order
 * @return whether the parsing was successful
 */
static inline bool objectFile_parseMachOImpl(struct objectFile* self,
                                             void*              baseAddress,
                                             bool               bytesSwapped) {
    struct mach_header*  header = baseAddress;
    struct load_command* lc     = (void*) header + sizeof(struct mach_header);
    const  uint32_t      ncmds  = macho_maybeSwap(32, bytesSwapped, header->ncmds);

    for (size_t i = 0; i < ncmds; ++i) {
        bool result = true;
        switch (macho_maybeSwap(32, bytesSwapped, lc->cmd)) {
            case LC_SEGMENT:
                result = objectFile_handleSegment(self, (void*) lc, baseAddress, bytesSwapped);
                break;

            case LC_SYMTAB:
                result = macho_parseSymtab((void*) lc, baseAddress, 0, bytesSwapped, false, NULL, objectFile_addFunctionCallback, self);
                break;

            case LC_UUID:
                memcpy(&self->uuid, &((struct uuid_command*) ((void*) lc))->uuid, 16);
                result = true;
                break;
        }
        if (!result) {
            return false;
        }
        lc = (void*) lc + macho_maybeSwap(32, bytesSwapped, lc->cmdsize);
    }
    return true;
}

/**
 * @brief Parses the Mach-O file into the given object file object.
 *
 * The Mach-O file needs to be an object file or a dSYM companion file to be parsed.
 * Optionally, it may be in a fat archive.
 *
 * @param self the object file object
 * @param buffer the buffer of the Mach-O file
 * @return whether the parsing was successful
 */
static inline bool objectFile_parseMachO(struct objectFile* self,
                                         void*              buffer) {
    if (buffer == NULL) return false;

    struct mach_header* header = buffer;

    if (header->magic == MH_MAGIC    || header->magic == MH_CIGAM ||
        header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64) {
        const uint32_t fileType = macho_maybeSwap(32, header->magic == MH_CIGAM || header->magic == MH_CIGAM_64, header->filetype);
        if (fileType != MH_OBJECT && fileType != MH_DSYM) {
            return false;
        }
    }

    bool success = false;
    switch (header->magic) {
        case MH_MAGIC: success = objectFile_parseMachOImpl(self, buffer, false); break;
        case MH_CIGAM: success = objectFile_parseMachOImpl(self, buffer, true);  break;

        case MH_MAGIC_64: success = objectFile_parseMachOImpl64(self, buffer, false); break;
        case MH_CIGAM_64: success = objectFile_parseMachOImpl64(self, buffer, true);  break;

        case FAT_MAGIC:
        case FAT_MAGIC_64: return objectFile_parseMachO(self, macho_parseFat(buffer, false, self->name));

        case FAT_CIGAM:
        case FAT_CIGAM_64: return objectFile_parseMachO(self, macho_parseFat(buffer, true, self->name));
    }

    if (success && self->debugLine.size > 0) {
        dwarf_parseLineProgram(self->debugLine,
                               self->debugLineStr,
                               self->debugStr,
                               self->debugInfo,
                               self->debugAbbrev,
                               self->debugStrOffsets,
                               objectFile_dwarfLineCallback, self);
    }
    return success;
}

bool objectFile_parseBuffer(struct objectFile* self, void* buffer) {
    const bool result = objectFile_parseMachO(self, buffer);
    if (!result) {
        for (size_t i = 0; i < self->ownFunctions.count; ++i) {
            function_destroy(&self->ownFunctions.content[i]);
        }
        vector_function_clear(&self->ownFunctions);
    } else {
        qsort(self->lineInfos.content, self->lineInfos.count, sizeof(struct dwarf_lineInfo), objectFile_dwarfLineInfoSortCompare);
        qsort(self->ownFunctions.content, self->ownFunctions.count, sizeof(struct function), objectFile_functionCompare);
    }
    return result;
}

bool objectFile_parse(struct objectFile* self) {
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
    const bool success = (off_t) count == fileStats.st_size && objectFile_parseBuffer(self, buffer);
    free(buffer);
    return success;
}


void objectFile_destroy(struct objectFile* self) {
    for (size_t i = 0; i < self->ownFunctions.count; ++i) {
        function_destroy(&self->ownFunctions.content[i]);
    }
    vector_function_destroy(&self->ownFunctions);
    for (size_t i = 0; i < self->lineInfos.count; ++i) {
        dwarf_lineInfo_destroy(&self->lineInfos.content[i]);
    }
    vector_dwarfLineInfo_destroy(&self->lineInfos);
    free((void*) self->mainSourceFileCache);
    free(self->sourceFile);
    free(self->directory);
    free(self->name);
}

void objectFile_delete(struct objectFile * self) {
    objectFile_destroy(self);
    free(self);
}
