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

#ifndef machoFile_h
#define machoFile_h

#include <stdbool.h>
#include <misc/numberContainers.h>

#include "containers_funcFile.h"
#include "objectFile.h"
#include "TLVDescriptor.h"
#include "../binaryFile.h"

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
    uint64_t tlvSize;
    /** Information about the dSYM bundle file.                                 */
    struct {
        /** Whether the file was already tried to be deducted. */
        bool triedParsing;
        /** The object file representation.                    */
        struct objectFile* file;
    } dSYMFile;
    /** The UUID of the represented Mach-O file.                                */
    uint8_t uuid[16];
    
    /** The functions mapped to their object file.                              */
    vector_pairFuncFile_t functions;
    /** The start addresses of the contained functions.                         */
    vector_uint64_t functionStarts;
    /** The contained thread-local value descriptors.                           */
    vector_TLVDescriptor_t tlvs;
};

/**
 * Initializes the given Mach-O file structure.
 *
 * @param self the Mach-O file structure to be initialized
 */
void machoFile_create(struct machoFile* self);

bool machoFile_parseShallow(struct machoFile* self);

/**
 * Stores all debug information that is possible to deduct about the given
 * address into the given callstack frame object.
 *
 * @param self the binary file the given address is in
 * @param address the address about which to find debug information
 * @param frame the callstack frame object to store the debug information in
 * @return whether it was possible to deduct some debug information
 */
bool machoFile_addr2String(struct machoFile* self, const void* address, struct callstack_frame* frame);

/**
 * Tries to fill the given function info structure with the information for the
 * function of the given name.
 *
 * @param self the Mach-O file abstraction structure
 * @param functionName the name of the desired function
 * @param info the info structure to be filled
 * @return whether the function was found
 */
bool machoFile_getFunctionInfo(struct machoFile* self, const char* functionName, struct functionInfo* info);

/**
 * Returns the contained thread-local storage regions.
 *
 * @param self the Mach-O file abstraction structure
 * @return the contained thread-local storage regions
 */
vector_pair_ptr_t machoFile_getTLSRegions(struct machoFile* self);

/**
 * Loads and parses the Mach-O file represented by the given Mach-O file
 * abstraction object.
 *
 * @param self the Mach-O file abstraction object
 * @return whether the file was parsed successfully
 */
bool machoFile_parse(struct machoFile* self);

/**
 * Deinitializes the given binary file structure if it is a Mach-O file
 * structure.
 *
 * @param self the binary file structure to be deinitialized
 */
void machoFile_destroy(struct machoFile* self);

/**
 * Deinitializes and `free`s the given binary file structure if it is a Mach-O
 * file structure.
 *
 * @param self the binary file structure to be deleted
 */
void machoFile_delete(struct machoFile* self);

/**
 * Clears the caches created by the Mach-O binary file implementation.
 */
void machoFile_clearCaches(void);

#endif /* machoFile_h */
