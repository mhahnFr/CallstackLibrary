/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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

#ifndef dwarfLineInfo_h
#define dwarfLineInfo_h

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * This structure represents a DWARF line program entry.
 */
struct dwarf_lineInfo {
    /** The address.       */
    uint64_t address;
    /** The line number.   */
    uint64_t line;
    /** The column number. */
    uint64_t column;
    /** The isa value.     */
    uint64_t isa;
    /** The discriminator. */
    uint64_t discriminator;
    
    /**
     * @brief The allocated full file name.
     *
     * If it is `NULL` the main source file was referenced.
     */
    const char* fileName;
    
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
 * Destroys the given DWARF line info.
 *
 * @param self the line info to be destroyed
 */
static inline void dwarf_lineInfo_destroy(struct dwarf_lineInfo* self) {
    free((void*) self->fileName);
}

#endif /* dwarfLineInfo_h */
