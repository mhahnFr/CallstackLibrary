/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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

#ifndef machoFile_h
#define machoFile_h

#include <stddef.h>

#include "../binaryFile.h"
#include "../debugInfo.h"
#include "../UInt64Vector.h"

#include "objectFile.h"
#include "FunctionVector.h"
#include "OptionalFuncFilePair.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This structure represents a Mach-O binary file.
 */
struct machoFile {
    /** The super part of this structure.                             */
    struct binaryFile _;
    
    /** The address offset between Mach-O file and loaded executable. */
    uint64_t addressOffset;
    bool inMemory;
    uint64_t linkedit_vmaddr;
    uint64_t linkedit_fileoff;
    uint64_t text_vmaddr;
    
    /** Pointer to the private part of this object.                   */
    void* priv;
};

/**
 * Allocates and initializes a Mach-O file structure.
 *
 * @return the allocated Mach-I file structure or `NULL` on error
 */
struct machoFile * machoFile_new(const char* fileName);

/**
 * Returns the represented Mach-O file structure from the given binary
 * file structure or `NULL` if it does not represent a Mach-O file structure.
 *
 * @param self the binary file structure to be casted
 * @return the represented Mach-O file structure or `NULL`
 */
static inline struct machoFile* machoFileOrNull(struct binaryFile * self) {
    return (struct machoFile*) (self->type == MACHO_FILE ? self->concrete : NULL);
}

/* Heavily WIP. */
bool machoFile_addr2String(struct binaryFile* self, void* address, struct callstack_frame* frame);

/**
 * Parses the Mach-O file represented by the given structure using the
 * given base address.
 *
 * @param self the Mach-O file structure representing the file to be parsed
 * @param baseAddress the base address of the Mach-O file to parse
 * @return whether the parsing was successful
 */
bool machoFile_parseFile(struct machoFile * self, void * baseAddress);

/**
 * Adds the givne object file structure to the given Mach-O file structure.
 *
 * @param self the Mach-O file structure
 * @param file the object file structure
 */
void machoFile_addObjectFile(struct machoFile *  self,
                             struct objectFile * file);

/**
 * Adds the given function to the given Mach-O file structure.
 *
 * @param self the Mach-O file structure
 * @param function the function to be added
 */
void machoFile_addFunction(struct machoFile* self, struct function function);

/**
 * Creates a debug info for the given address if possible.
 *
 * @param self the Mach-O file structure
 * @param address the address whose debug info to deduct
 * @return the optionally deducted debug information
 */
optional_debugInfo_t machoFile_getDebugInfo(struct machoFile* self,
                                            void*             address);

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

bool machoFile_isLoaded(struct machoFile* self);

/**
 * Initializes the given Mach-O file structure.
 *
 * @param self the Mach-O file structure to be initialized
 */
static inline void machoFile_create(struct machoFile* self, const char* fileName) {
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
    self->inMemory         = machoFile_isLoaded(self);
}

void machoFile_clearCaches(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* machoFile_h */
