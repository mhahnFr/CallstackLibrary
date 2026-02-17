/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2026  mhahnFr
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

#include "objectFile.h"

#include <file/pathUtils.h>
#include <mach-o/loader.h>
#include <macho/fat_handler.h>
#include <macho/macho_utils.h>

#include "macho_parser.h"
#include "../binaryFile.h"
#include "../bounds.h"
#include "../loader.h"
#include "../dwarf/parser.h"

struct objectFile* objectFile_new(void) {
    struct objectFile* self = malloc(sizeof(struct objectFile));
    if (self != NULL) {
        *self = objectFile_initializer;
    }
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
 * @brief Returns how the two given symbols compare.
 *
 * Sorted ascendingly.
 *
 * @param a the left-hand side value
 * @param b the right-hand side value
 * @return the difference between the two given symbols
 */
static inline int objectFile_symbolCompare(const struct symbol* a, const struct symbol* b) {
    return strcmp(a->linkedName, b->linkedName);
}

/**
 * Finds and returns the symbol with the given name deducted from the
 * represented object file.
 *
 * @param self the object file object
 * @param name the name of the desired symbol
 * @return the optionally found symbol
 */
static inline optional_symbol_t objectFile_findOwnSymbol(struct objectFile* self, const char* name) {
    optional_symbol_t toReturn = { .has_value = false };

    const struct symbol searched = (struct symbol) { .linkedName = (char*) name };
    const struct symbol* it = vector_search(&self->ownSymbols, &searched, objectFile_symbolCompare);
    if (it != NULL) {
        toReturn = (struct optional_symbol) { true, *it };
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

/**
 * Parses the given object file if it has not already been parsed.
 *
 * @param self the object file
 * @return whether the object file has been parsed
 */
static inline bool objectFile_maybeParse(struct objectFile* self) {
    return self->parsed || ((self->parsed = objectFile_parse(self)));
}

optional_debugInfo_t objectFile_getDebugInfo(struct objectFile* self, const uint64_t address, const struct symbol symbol) {
#define ERROR_RETURN (optional_debugInfo_t) { .has_value = false };

    if (!objectFile_maybeParse(self)) {
        return ERROR_RETURN;
    }
    uint64_t lineAddress;
    uint64_t symbolBegin;
    if (self->isDsymBundle) {
        lineAddress = address;
        symbolBegin = symbol.startAddress;
    } else {
        const optional_symbol_t ownSymbol = objectFile_findOwnSymbol(self, symbol.linkedName);
        if (!ownSymbol.has_value) {
            return ERROR_RETURN;
        }
        lineAddress = ownSymbol.value.startAddress + address - symbol.startAddress;
        symbolBegin = ownSymbol.value.startAddress;
    }

    const struct dwarf_lineInfo tmp = (struct dwarf_lineInfo) { .address = lineAddress };
    const struct dwarf_lineInfo* closest = upper_bound(&tmp,
                                                       self->lineInfos.content,
                                                       self->lineInfos.count,
                                                       sizeof(struct dwarf_lineInfo),
                                                       objectFile_dwarfLineInfoSortCompare);
    if (closest == NULL || closest->address < symbolBegin
        || (symbol.length != 0 && closest->address >= symbolBegin + symbol.length)) {
        return ERROR_RETURN;
    }
    if (closest->sourceFile.fileName != NULL && closest->sourceFile.fileNameRelative == NULL && closest->sourceFile.fileNameAbsolute == NULL) {
        struct dwarf_lineInfo* mutableClosest = (struct dwarf_lineInfo*) closest;
        mutableClosest->sourceFile.fileNameRelative = path_toRelativePath(closest->sourceFile.fileName);
        mutableClosest->sourceFile.fileNameAbsolute = path_toAbsolutePath(closest->sourceFile.fileName);
    }
    return (optional_debugInfo_t) {
        true, (struct debugInfo) {
            symbol, (optional_sourceFileInfo_t) {
                true, (struct sourceFileInfo) {
                    closest->line,
                    closest->column,
                    closest->sourceFile.fileName == NULL ? objectFile_getSourceFileName(self) : closest->sourceFile.fileName,
                    closest->sourceFile.fileName == NULL ? self->mainSourceFileCacheRelative : closest->sourceFile.fileNameRelative,
                    closest->sourceFile.fileName == NULL ? self->mainSourceFileCacheAbsolute : closest->sourceFile.fileNameAbsolute,
                    binaryFile_isOutdated(closest->sourceFile)
                }
            }
        }
    };
#undef ERROR_RETURN
}

uint8_t* objectFile_getUUID(struct objectFile* self) {
    objectFile_maybeParse(self);
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
 * Callback function adding the given symbol to the list of the object file.
 *
 * @param self the object file
 * @param pair the symbol to be added
 */
static inline void objectFile_addSymbolCallback(struct objectFile* self, pair_symbolFile_t pair) {
    vector_push_back(&self->ownSymbols, pair.first);
}

/**
 * Generates an implementation for actually parsing Mach-O object files.
 *
 * @param bits the amount of bits the implementation should be generated for
 * @param suffix the optional suffix for the native data structures
 */
#define objectFile_parseMachOImplFunc(bits, suffix)                                           \
static inline void objectFile_parseMachOImpl##bits(struct objectFile* self,                   \
                                                   const void*        baseAddress,            \
                                                   const bool         bytesSwapped) {         \
    macho_iterateSegments(baseAddress, bytesSwapped, suffix, {                                \
        switch (macho_maybeSwap(32, bytesSwapped, loadCommand->cmd)) {                        \
            case LC_SEGMENT##suffix:                                                          \
                macho_iterateSections((void*) loadCommand, bytesSwapped, suffix,              \
                    objectFile_handleSection(self, (struct lcs_section) {                     \
                        baseAddress + macho_maybeSwap(32, bytesSwapped, section->offset),     \
                        macho_maybeSwap(bits, bytesSwapped, section->size)                    \
                    }, section->segname, section->sectname);                                  \
                )                                                                             \
                break;                                                                        \
                                                                                              \
            case LC_SYMTAB: {                                                                 \
                struct machoParser parser = machoParser_create(                               \
                    (void*) loadCommand, baseAddress, 0,                                      \
                    bytesSwapped, (bits) == 64,                                               \
                    (machoParser_addSymbol) objectFile_addSymbolCallback, self                \
                );                                                                            \
                TRY({                                                                         \
                    machoParser_parseSymbolTable(&parser);                                    \
                    machoParser_destroy(&parser);                                             \
                }, CATCH_ALL(_, {                                                             \
                    machoParser_destroy(&parser);                                             \
                    RETHROW;                                                                  \
                }))                                                                           \
                break;                                                                        \
            }                                                                                 \
                                                                                              \
            case LC_UUID:                                                                     \
                memcpy(&self->uuid, &((struct uuid_command*) (void*) loadCommand)->uuid, 16); \
                break;                                                                        \
                                                                                              \
            default: break;                                                                   \
        }                                                                                     \
    })                                                                                        \
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
static inline void objectFile_parseMachO(struct objectFile* self, const void* buffer) {
    if (buffer == NULL) {
        M_THROW(empty, "No buffer to be parsed given");
    }

    const struct mach_header* header = buffer;

    if (header->magic == MH_MAGIC    || header->magic == MH_CIGAM ||
        header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64) {
        const uint32_t fileType = macho_maybeSwap(32, header->magic == MH_CIGAM || header->magic == MH_CIGAM_64, header->filetype);
        if (fileType != MH_OBJECT && fileType != MH_DSYM) {
            M_THROW(unsupported, "Mach-O file to parse is neither a dSYM bundle file nor an object file");
        }
    }

    switch (header->magic) {
        case MH_MAGIC:    objectFile_parseMachOImpl32(self, buffer, false); break;
        case MH_CIGAM:    objectFile_parseMachOImpl32(self, buffer, true);  break;
        case MH_MAGIC_64: objectFile_parseMachOImpl64(self, buffer, false); break;
        case MH_CIGAM_64: objectFile_parseMachOImpl64(self, buffer, true);  break;

        case FAT_MAGIC:
        case FAT_MAGIC_64: return objectFile_parseMachO(self, macho_parseFat(buffer, false, self->name));

        case FAT_CIGAM:
        case FAT_CIGAM_64: return objectFile_parseMachO(self, macho_parseFat(buffer, true, self->name));

        default: break;
    }

    if (self->debugLine.size > 0) {
        dwarf_parseLineProgram(self->debugLine,
                               self->debugLineStr,
                               self->debugStr,
                               self->debugInfo,
                               self->debugAbbrev,
                               self->debugStrOffsets,
                               objectFile_dwarfLineCallback, self);
    }
}

bool objectFile_parseBuffer(struct objectFile* self, const void* buffer) {
    bool result = true;
    TRY({
        objectFile_parseMachO(self, buffer);
        vector_sort(&self->lineInfos, objectFile_dwarfLineInfoSortCompare);
        vector_sort(&self->ownSymbols, objectFile_symbolCompare);
    }, CATCH_ALL(_, {
        vector_destroyWithPtr(&self->ownSymbols, symbol_destroy);
        vector_init(&self->ownSymbols);
        result = false;
    }));
    return result;
}

bool objectFile_parse(struct objectFile* self) {
    const time_t lastModified = self->lastModified;
    return loader_loadFileAndExecuteTime(self->name, lastModified == 0 ? NULL : &lastModified, (union loader_parserFunction) {
        (loader_parser) objectFile_parseBuffer
    }, false, self);
}


void objectFile_destroy(struct objectFile* self) {
    vector_destroyWithPtr(&self->ownSymbols, symbol_destroy);
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
