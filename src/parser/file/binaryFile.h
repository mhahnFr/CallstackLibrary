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

#ifndef binaryFile_h
#define binaryFile_h

#include <stdbool.h>

#include <callstack_frame.h>
#include <functionInfo/functionInfo.h>

#include "dwarf/dwarf_lineInfo.h"

/**
 * This enumeration contains the supported types of executable files.
 */
enum binaryFileType {
    /** Represents a Mach-O binary file. */
    MACHO_FILE,
    /** Represents an ELF binary file.   */
    ELF_FILE
};

/**
 * This structure represents a generic binary executable file.
 */
struct binaryFile {
    /** The type of this binary file.                                    */
    enum binaryFileType type;
    /** A pointer to the concrete structure.                             */
    void * concrete;
    
    /** Indicates whether this file has already been parsed.             */
    bool parsed;
    /** Indicates whether the represented image is loaded by the system. */
    bool inMemory;

    /** The name of the represented binary file.                         */
    const char * fileName;
    
    /** The start address in memory of the represented binary file.      */
    const void* startAddress;
    /** The relocation offset of the binary file.                        */
    uintptr_t relocationOffset;
    
    /**
     * @brief The translating method.
     *
     * Attempts to translate the given address into the given callstack
     * frame object.
     *
     * Returns whether the address could be translated.
     */
    bool (*addr2String)(struct binaryFile*, void*, struct callstack_frame*);
    bool (*getFunctionInfo)(struct binaryFile*, const char*, struct functionInfo*);
    /** The appropriate deinitializing method.                           */
    void (*destroy)(struct binaryFile*);
    /** The appropriate deleting method.                                 */
    void (*deleter)(struct binaryFile*);
};

/**
 * Allocates a new concrete binary file structure.
 *
 * @param fileName the name of the represented binary file
 * @param startAddress the start address of the represented binary file
 */
struct binaryFile* binaryFile_new(const char* fileName, const void* startAddress);

/**
 * Initializes the given binary file structure.
 *
 * @param self the binary file structure to be initialized
 */
static inline void binaryFile_create(struct binaryFile* self) {
    self->fileName = NULL;
    self->parsed   = false;
    self->inMemory = false;
}

/**
 * Clears the caches created by the binary file implementations.
 */
void binaryFile_clearCaches(void);

/**
 * Returns whether the given source file is outdated, that is, whether it has been edited since being referred.
 *
 * @param file the file information to be checked
 * @return whether the file is outdated
 */
bool binaryFile_isOutdated(struct dwarf_sourceFile file);

#endif /* binaryFile_h */
