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

#ifndef binaryFile_h
#define binaryFile_h

#include <callstack_frame.h>
#include <stdbool.h>
#include <functionInfo/functionInfo.h>

#include "vector_pair_ptr.h"
#include "dwarf/lineInfo/lineInfo.h"

/**
 * This structure represents a generic binary executable file.
 */
struct binaryFile {
    /** Indicates whether this file has already been parsed.             */
    bool parsed,
    /** Indicates whether the represented image is loaded by the system. */
         inMemory,
    /**
     * Indicates whether the represented image is the located within the image
     * of the CallstackLibrary.
     */
         isSelf;

    /**
     * The different versions of the file name.
     */
    struct FileName {
        /** The original file name as given by the system. */
        char* original,
        /** The absolute file name, if available.          */
            * absolute,
        /** The relative file name, if available.          */
            * relative;
    } fileName;
    /** The start address in memory of the represented binary file.      */
    const void* startAddress,
    /** The end address in memory of the represented binary file.        */
              * end;
    /** The relocation offset of the binary file.                        */
    uintptr_t relocationOffset;
    /** The regions for global storage in this binary file.              */
    vector_pair_ptr_t regions;
};

/**
 * A search function prototype suitable for searching a symbol in the symbol table.
 */
typedef const void* (*binaryFile_searchFunction)(const void*, const void*, size_t, size_t, int (*)(const void*, const void*));

/**
 * Allocates a new concrete binary file structure.
 *
 * @param fileName the name of the represented binary file
 * @param startAddress the start address of the represented binary file
 * @return the allocated binary file structure
 */
struct binaryFile* binaryFile_new(const char* fileName, const void* startAddress);

/**
 * Deducts the debug information available for the given address and stores it
 * in the given callstack frame.
 *
 * @param self the binary file abstraction structure
 * @param address the address to translate
 * @param frame the callstack frame structure to store the information in
 * @return whether debug information was deducted successfully
 */
bool binaryFile_addr2String(struct binaryFile* self, const void* address, struct callstack_frame* frame);

/**
 * Retrieves the function information available in the given binary file object.
 *
 * @param self the binary file object
 * @param functionName the name of the function (as it was linked) to look up
 * @param info the function info structure to be filled
 * @return whether the function was found
 */
bool binaryFile_getFunctionInfo(struct binaryFile* self, const char* functionName, struct functionInfo* info);

/**
 * @brief Returns the regions found in the given binary file.
 *
 * Parses the file if necessary before returning the regions.
 *
 * @param self the binary file
 * @return the vector with the regions found in the given binary file
 */
vector_pair_ptr_t* binaryFile_getRegions(struct binaryFile* self);

/**
 * Queries the debug information for the symbol of the given address.
 *
 * @param self the binary file
 * @param symbolAddress the address of the desired symbol
 * @param frame the @c callstack_frame structure to place the debug info in
 * @return whether a symbol was found
 */
bool binaryFile_getSymbolInfo(struct binaryFile* self, const void* symbolAddress, struct callstack_frame* frame);

/**
 * Sorts the regions found in the given binary file.
 *
 * @param self the binary file
 */
void binaryFile_sortRegions(struct binaryFile* self);

/**
 * Returns the thread-local storage regions of the given binary file.
 *
 * @param self the binary file abstraction object
 * @return the thread-local storage regions in the given binary file
 */
vector_pair_ptr_t binaryFile_getTLSRegions(struct binaryFile* self);

/**
 * @brief Parses this binary file if it has not been (successfully) parsed.
 *
 * Stores whether the parsing succeeded.
 *
 * @param self the binary file to be parsed
 * @return whether the parsing was successful
 */
bool binaryFile_maybeParse(struct binaryFile* self);

/**
 * Clears the caches created by the binary file implementations.
 */
void binaryFile_clearCaches(void);

/**
 * Returns whether the given source file is outdated, that is, whether it has
 * been edited since being referred.
 *
 * @param file the file information to be checked
 * @return whether the file is outdated
 */
bool binaryFile_isOutdated(struct dwarf_sourceFile file);

/**
 * Destroys the given binary file.
 *
 * @param self the binary file abstraction structure to destroy
 */
void binaryFile_destroy(struct binaryFile* self);

/**
 * Destroys and deallocates the given binary file.
 *
 * @param self the binary file structure to delete
 */
void binaryFile_delete(struct binaryFile* self);

/**
 * Executes the named function of the binary file class.
 *
 * @param self the binary file abstraction structure
 * @param name the name of the function to execute
 * @param ... the arguments to pass to the requested function
 */
#define BINARY_FILE_SUPER(self, name, ...) binaryFile_##name((struct binaryFile*) (self) __VA_OPT__(,) __VA_ARGS__)

#endif /* binaryFile_h */
