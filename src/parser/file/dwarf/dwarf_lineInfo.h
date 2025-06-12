/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2025  mhahnFr
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

#ifndef dwarfLineInfo_h
#define dwarfLineInfo_h

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief This structure represents a source file reference.
 *
 * Unavailable fields are set to `0` and `NULL`, respectively.
 */
struct dwarf_sourceFile {
    /** The allocated source file name.          */
    const char* fileName;
    /** The allocated relative source file name. */
    const char* fileNameRelative;
    /** The allocated absolute source file name. */
    const char* fileNameAbsolute;
    /** The timestamp of the last modification.  */
    uint64_t timestamp;
    /** The size of the file.                    */
    uint64_t size;
};

/**
 * This structure represents a DWARF line program entry.
 */
struct dwarf_lineInfo {
    /** The address.                                                */
    uint64_t address;
    /** The line number.                                            */
    uint64_t line;
    /** The column number.                                          */
    uint64_t column;
    /** The isa value.                                              */
    uint64_t isa;
    /** The discriminator.                                          */
    uint64_t discriminator;

    /** The referred source file.                                   */
    struct dwarf_sourceFile sourceFile;

    /** Whether this position is a recommended breakpoint position. */
    bool isStmt;
    /** Whether this position is a basic block.                     */
    bool basicBlock;
    /** Whether this position is the end of a sequence.             */
    bool endSequence;
    /** Whether this position is the end of a prologue.             */
    bool prologueEnd;
    /** Whether this position is the beginning of an epilogue.      */
    bool epilogueBegin;
};

/**
 * Destroys the given DWARF file info, passed by value.
 *
 * @param self the line info to be destroyed
 */
static inline void dwarf_lineInfo_destroyValue(const struct dwarf_lineInfo self) {
    free((void*) self.sourceFile.fileName);
    free((void*) self.sourceFile.fileNameRelative);
    free((void*) self.sourceFile.fileNameAbsolute);
}

/**
 * Destroys the given DWARF line info.
 *
 * @param self the line info to be destroyed
 */
static inline void dwarf_lineInfo_destroy(const struct dwarf_lineInfo* self) {
    dwarf_lineInfo_destroyValue(*self);
}

#endif /* dwarfLineInfo_h */
