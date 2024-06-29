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

#ifndef machoFile_h
#define machoFile_h

#include <stdbool.h>
#include <stddef.h>

#include "objectFile.h"
#include "vector_pair_funcFile.h"

#include "../binaryFile.h"
#include "../debugInfo.h"
#include "../vector_function.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This structure represents a Mach-O binary file.
 */
struct machoFile {
    /** The super part of this structure.                                       */
    struct binaryFile _;
    
    /** The address offset between Mach-O file and loaded executable.           */
    uint64_t addressOffset;
    /** The VM address of the linkedit segment.                                 */
    uint64_t linkedit_vmaddr;
    /** The file offset of the linkedit segment.                                */
    uint64_t linkedit_fileoff;
    /** The VM address of the text segment.                                     */
    uint64_t text_vmaddr;
    /** Information about the dSYM bundle file.                                 */
    struct {
        /** Whether the file was already tried to be deducted. */
        bool triedParsing;
        /** The object file representation.                    */
        struct objectFile* file;
    } dSYMFile;
    /** The UUID of the represented Mach-O file.                                */
    uint8_t uuid[16];
    
    vector_pairFuncFile_t functions;
};

/**
 * Allocates and initializes a Mach-O file structure.
 *
 * @param fileName the name of the file
 * @return the allocated Mach-O file structure or `NULL` on error
 */
struct machoFile * machoFile_new(const char* fileName);

/**
 * Initializes the given Mach-O file structure.
 *
 * @param self the Mach-O file structure to be initialized
 */
void machoFile_create(struct machoFile* self, const char* fileName);

/**
 * Returns the represented Mach-O file structure from the given binary
 * file structure or `NULL` if it does not represent a Mach-O file structure.
 *
 * @param self the binary file structure to be casted
 * @return the represented Mach-O file structure or `NULL`
 */
static inline struct machoFile* machoFileOrNull(struct binaryFile * self) {
    return self->type == MACHO_FILE ? self->concrete : NULL;
}

/**
 * Stores all debug information that is possible to deduct about the given address 
 * into the given callstack frame object.
 *
 * @param self the binary file the given address is in
 * @param address the address about which to find debug information
 * @param frame the callstack frame object to store the debug information in
 * @return whether it was possible to deduct some debug information
 */
bool machoFile_addr2String(struct binaryFile* self, void* address, struct callstack_frame* frame);

/**
 * Deinitializes the given binary file structure if it is a Mach-O file structure.
 *
 * @param self the binary file structure to be deinitialized
 */
void machoFile_destroy(struct binaryFile * self);

/**
 * Deinitializes and `free`s the given binary file structure if it is a
 * Mach-O file structure.
 *
 * @param self the binary file structure to be deleted
 */
void machoFile_delete(struct binaryFile * self);

/**
 * Clears the caches created by the Mach-O binary file implementation.
 */
void machoFile_clearCaches(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* machoFile_h */
