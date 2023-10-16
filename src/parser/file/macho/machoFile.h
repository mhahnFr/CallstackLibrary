/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023  mhahnFr
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
#include "../UInt64Vector.h"
#include "objectFile.h"

/**
 * This structure represents a Mach-O binary file.
 */
struct machoFile {
    /** The super part of this structure.                             */
    struct binaryFile _;
    
    /** The address offset between Mach-O file and loaded executable. */
    uint64_t addressOffset;
    /** The contained object files.                                   */
    struct objectFile * objectFiles;
    /** Vector with all function start addresses found in this file.  */
    struct vector_uint64_t functionStarts;
};

/**
 * Allocates and initializes a Mach-O file structure.
 *
 * @return the allocated Mach-I file structure or `NULL` on error
 */
struct machoFile * machoFile_new(void);

/**
 * Initializes the given Mach-O file structure.
 *
 * @param self the Mach-O file structure to be initialized
 */
void machoFile_create(struct machoFile * self);

/**
 * Returns the represented Mach-O file structure from the given binary
 * file structure or `NULL` if it does not represent a Mach-O file structure.
 *
 * @param self the binary file structure to be casted
 * @return the represented Mach-O file structure or `NULL`
 */
static inline struct machoFile * machoFileOrNull(struct binaryFile * self) {
    return self->type == MACHO_FILE ? self->concrete : NULL;
}

/* Heavily WIP. */
bool machoFile_addr2String(struct binaryFile *      self,
                                  Dl_info *         info,
                                  void *            address,
                           struct callstack_frame * frame);

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

#endif /* machoFile_h */
