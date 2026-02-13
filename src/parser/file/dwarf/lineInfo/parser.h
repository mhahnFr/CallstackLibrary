/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2026  mhahnFr
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

#ifndef CALLSTACKLIBRARY_DWARF_LINEINFOPARSER_H
#define CALLSTACKLIBRARY_DWARF_LINEINFOPARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Represents a DWARF line information parser.
 */
struct dwarf_lineInfoParser {
    /** The address of the current position.                              */
    uint64_t address,
    /** The operation index.                                              */
             opIndex,
    /** The number of the file currently used.                            */
             file,
    /** The current line number.                                          */
             line,
    /** The current column number.                                        */
             column,
    /** The current isa value.                                            */
             isa,
    /** The discriminator of the current position.                        */
             discriminator;

    /** Whether the position is a recommended breakpoint position.        */
    bool isStmt,
    /** Whether the current position is a basic code block.               */
         basicBlock,
    /** Whether the current position is the end of a sequence.            */
         endSequence,
    /** Whether the current position is the end of the function prologue. */
         prologueEnd,
    /** Whether the current position is the beginning of the epilogue.    */
         epilogueBegin;

    /** The main DWARF parser. */
    struct dwarf_parser* parser;
};

/**
 * Initializing expression for the DWARF line information parser structure.
 *
 * @param defaultIsStmt whether the @c stmt value is @c true by default
 * @param parser the main DWARF parser
 */
#define dwarf_lineInfoParser_initializer(defaultIsStmt, parser) \
(struct dwarf_lineInfoParser) {                                 \
    0, 0, 1, 1, 0, 0, 0,                                        \
    (defaultIsStmt), false, false, false, false,                \
    parser                                                      \
}

/**
 * Handles a default operation entry.
 *
 * @param self the DWARF line information parser object
 * @param opCode the operation code to be handled
 */
void dwarf_lineInfoParser_handleDefaultEntry(struct dwarf_lineInfoParser* self, uint8_t opCode);

/**
 * Handles a single instruction operation code.
 *
 * @param self the DWARF line information parser
 * @param counter the counter used as offset into the memory buffer
 * @param opCode the operation code to be handled
 */
void dwarf_lineInfoParser_handleSingeInstruction(struct dwarf_lineInfoParser* self, size_t* counter, uint8_t opCode);

/**
 * Handles a special operation code.
 *
 * @param self the DWARF line information parser object
 * @param counter the counter used as offset into the memory buffer
 * @param length the length of the operation
 * @param opCode the operation code to be handled
 */
void dwarf_lineInfoParser_handleSpecialOperation(struct dwarf_lineInfoParser* self, size_t* counter, uint64_t length,
                                                 uint8_t opCode);

#endif //CALLSTACKLIBRARY_DWARF_LINEINFOPARSER_H