/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
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
#include <mach-o/loader.h>
#include <sys/stat.h>

#include <file/pathUtils.h>
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
    
    vector_push_back(&self->lineInfos, info);
}

/**
 * @brief Returns how the two given DWARF line infos compare.
 *
 * Sorted descendingly.
 *
 * @param lhs the left-hand side value
 * @param rhs the right-hand side value
 * @return @c 0 if the two infos compare equal, a value smaller or bigger than
 * @c 0 according to the sort order
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
 * Finds and returns the function with the given name deducted from the
 * represented object file.
 *
 * @param self the object file object
 * @param name the name of the desired function
 * @return the optionally found function
 */
static inline optional_function_t objectFile_findOwnFunction(struct objectFile* self, const char* name) {
    optional_function_t toReturn = { .has_value = false };

    const struct function searched = (struct function) { .linkedName = (char*) name };
    const struct function* it = vector_search(&self->ownFunctions, &searched, objectFile_functionCompare);
    if (it != NULL) {
        toReturn = (struct optional_function) { true, *it };
    }
    
    return toReturn;
}

/**
 * @brief Returns the full source file name found in the referencing Mach-O
 * executable / library file.
 *
 * @note Do not free the returned string.
 *
 * @param self the object file object
 * @return the full source file name or @c NULL if the allocation failed
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

    self->mainSourceFileCacheRelative = path_toRelativePath(toReturn);
    self->mainSourceFileCacheAbsolute = path_toAbsolutePath(toReturn);

    return self->mainSourceFileCache = toReturn;
}

optional_debugInfo_t objectFile_getDebugInfo(struct objectFile* self, const uint64_t address, const struct function function) {
    const optional_debugInfo_t toReturn = { .has_value = false };
    
    if (!self->parsed) {
        if (!((self->parsed = objectFile_parse(self)))) {
            return toReturn;
        }
    }
    uint64_t lineAddress;
    uint64_t functionBegin;
    if (self->isDsymBundle) {
        lineAddress = address;
        functionBegin = function.startAddress;
    } else {
        const optional_function_t ownFunction = objectFile_findOwnFunction(self, function.linkedName);
        if (!ownFunction.has_value) {
            return toReturn;
        }
        lineAddress = ownFunction.value.startAddress + address - function.startAddress;
        functionBegin = ownFunction.value.startAddress;
    }

    const struct dwarf_lineInfo tmp = (struct dwarf_lineInfo) { .address = lineAddress };
    const struct dwarf_lineInfo* closest = upper_bound(&tmp,
                                                       self->lineInfos.content,
                                                       self->lineInfos.count,
                                                       sizeof(struct dwarf_lineInfo),
                                                       objectFile_dwarfLineInfoSortCompare);
    if (closest == NULL || closest->address < functionBegin
        || (function.length != 0 && closest->address >= functionBegin + function.length)) {
        return toReturn;
    }
    if (closest->sourceFile.fileName != NULL && closest->sourceFile.fileNameRelative == NULL && closest->sourceFile.fileNameAbsolute == NULL) {
        struct dwarf_lineInfo* mutableClosest = (struct dwarf_lineInfo*) closest;
        mutableClosest->sourceFile.fileNameRelative = path_toRelativePath(closest->sourceFile.fileName);
        mutableClosest->sourceFile.fileNameAbsolute = path_toAbsolutePath(closest->sourceFile.fileName);
    }
    const char* fileName = closest->sourceFile.fileName == NULL ? objectFile_getSourceFileName(self) : closest->sourceFile.fileName;
    return (optional_debugInfo_t) {
        true, (struct debugInfo) {
            function, (optional_sourceFileInfo_t) {
                true, (struct sourceFileInfo) {
                    closest->line,
                    closest->column,
                    fileName,
                    closest->sourceFile.fileName == NULL ? self->mainSourceFileCacheRelative : closest->sourceFile.fileNameRelative,
                    closest->sourceFile.fileName == NULL ? self->mainSourceFileCacheAbsolute : closest->sourceFile.fileNameAbsolute,
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
 * @param segmentName the segment name the section is found in
 * @param sectionName the section's name
 */
static inline void objectFile_handleSection(struct objectFile* self,
                                            const struct lcs_section section,
                                            const char* segmentName,
                                            const char* sectionName) {
    if (strcmp("__DWARF", segmentName) == 0) {
        if (strcmp("__debug_line", sectionName) == 0) {
            self->debugLine = section;
        } else if (strncmp("__debug_line_str", sectionName, 16) == 0) {
            self->debugLineStr = section;
        } else if (strcmp("__debug_str", sectionName) == 0) {
            self->debugStr = section;
        } else if (strcmp("__debug_info", sectionName) == 0) {
            self->debugInfo = section;
        } else if (strcmp("__debug_abbrev", sectionName) == 0) {
            self->debugAbbrev = section;
        } else if (strncmp("__debug_str_offsets", sectionName, 16) == 0) {
            self->debugStrOffsets = section;
        }
    }
}

/**
 * Generates an implementation for an object file segment handling parsing function.
 *
 * @param bits the amount of bits the implementation should be generated for
 * @param suffix the optional suffix for the native data structures
 */
#define objectFile_handleSegmentFunc(bits, suffix)                                                 \
static inline bool objectFile_handleSegment##bits(struct objectFile*         self,                 \
                                                  struct segment_command##suffix* command,         \
                                                  void*                      baseAddress,          \
                                                  const bool                 bytesSwapped) {       \
    const uint32_t nsects = macho_maybeSwap(32, bytesSwapped, command->nsects);                    \
                                                                                                   \
    for (size_t i = 0; i < nsects; ++i) {                                                          \
        struct section##suffix* section = (void*) command + sizeof(struct segment_command##suffix) \
                                        + i * sizeof(struct section##suffix);                      \
        objectFile_handleSection(self, (struct lcs_section) {                                      \
            baseAddress + macho_maybeSwap(32, bytesSwapped, section->offset),                      \
            macho_maybeSwap(bits, bytesSwapped, section->size)                                     \
        }, section->segname, section->sectname);                                                   \
    }                                                                                              \
    return true;                                                                                   \
}

objectFile_handleSegmentFunc(32,)
objectFile_handleSegmentFunc(64, _64)

/**
 * The callback adding the function / object file pair to the object file
 * object passed via the variadic argument list.
 *
 * @param f the function / object file pair
 * @param args the arguments - should contain as first argument the object file object ot add the pair to
 */
static inline void objectFile_addFunctionCallback(struct pair_funcFile f, va_list args) {
    struct objectFile* self = va_arg(args, void*);

    vector_push_back(&self->ownFunctions, f.first);
}

/**
 * Generates an implementation for actually parsing Mach-O object files.
 *
 * @param bits the amount of bits the implementation should be generated for
 * @param suffix the optional suffix for the native data structures
 */
#define objectFile_parseMachOImplFunc(bits, suffix)                                                   \
static inline bool objectFile_parseMachOImpl##bits(struct objectFile* self,                           \
                                                   void*              baseAddress,                    \
                                                   const bool         bytesSwapped) {                 \
    struct mach_header##suffix* header = baseAddress;                                                 \
    struct load_command*   lc     = (void*) header + sizeof(struct mach_header##suffix);              \
    const  uint32_t        ncmds  = macho_maybeSwap(32, bytesSwapped, header->ncmds);                 \
                                                                                                      \
    for (size_t i = 0; i < ncmds; ++i) {                                                              \
        bool result = true;                                                                           \
        switch (macho_maybeSwap(32, bytesSwapped, lc->cmd)) {                                         \
            case LC_SEGMENT##suffix:                                                                  \
                result = objectFile_handleSegment##bits(self, (void*) lc, baseAddress, bytesSwapped); \
                break;                                                                                \
                                                                                                      \
            case LC_SYMTAB:                                                                           \
                result = macho_parseSymtab((void*) lc, baseAddress, 0, bytesSwapped, true, NULL,      \
                                           objectFile_addFunctionCallback, self);                     \
                break;                                                                                \
                                                                                                      \
            case LC_UUID:                                                                             \
                memcpy(&self->uuid, &((struct uuid_command*) (void*) lc)->uuid, 16);                  \
                result = true;                                                                        \
                break;                                                                                \
                                                                                                      \
            default: break;                                                                           \
        }                                                                                             \
        if (!result) {                                                                                \
            return false;                                                                             \
        }                                                                                             \
        lc = (void*) lc + macho_maybeSwap(32, bytesSwapped, lc->cmdsize);                             \
    }                                                                                                 \
    return true;                                                                                      \
}

objectFile_parseMachOImplFunc(32,)
objectFile_parseMachOImplFunc(64, _64)

/**
 * @brief Parses the Mach-O file into the given object file object.
 *
 * The Mach-O file needs to be an object file or a dSYM companion file to be
 * parsed. Optionally, it may be in a fat archive.
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
        case MH_MAGIC: success = objectFile_parseMachOImpl32(self, buffer, false); break;
        case MH_CIGAM: success = objectFile_parseMachOImpl32(self, buffer, true);  break;

        case MH_MAGIC_64: success = objectFile_parseMachOImpl64(self, buffer, false); break;
        case MH_CIGAM_64: success = objectFile_parseMachOImpl64(self, buffer, true);  break;

        case FAT_MAGIC:
        case FAT_MAGIC_64: return objectFile_parseMachO(self, macho_parseFat(buffer, false, self->name));

        case FAT_CIGAM:
        case FAT_CIGAM_64: return objectFile_parseMachO(self, macho_parseFat(buffer, true, self->name));

        default: break;
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
        vector_destroyWithPtr(&self->ownFunctions, function_destroy);
        vector_init(&self->ownFunctions);
    } else {
        vector_sort(&self->lineInfos, objectFile_dwarfLineInfoSortCompare);
        vector_sort(&self->ownFunctions, objectFile_functionCompare);
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
    vector_destroyWithPtr(&self->ownFunctions, function_destroy);
    vector_destroyWithPtr(&self->lineInfos, dwarf_lineInfo_destroy);
    free((void*) self->mainSourceFileCache);
    free((void*) self->mainSourceFileCacheRelative);
    free((void*) self->mainSourceFileCacheAbsolute);
    free(self->sourceFile);
    free(self->directory);
    free(self->name);
}

void objectFile_delete(struct objectFile * self) {
    objectFile_destroy(self);
    free(self);
}
