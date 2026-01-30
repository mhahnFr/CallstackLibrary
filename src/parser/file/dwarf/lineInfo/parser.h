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

struct dwarf_lineInfoParser {
    uint64_t address, opIndex, file, line, column, isa, discriminator;
    bool isStmt, basicBlock, endSequence, prologueEnd, epilogueBegin;
    struct dwarf_parser* parser;
};

#define dwarf_lineInfoParser_initializer(defaultIsStmt, parser) \
(struct dwarf_lineInfoParser) {                                 \
    0, 0, 1, 1, 0, 0, 0,                                        \
    (defaultIsStmt), false, false, false, false,                \
    parser                                                      \
}

void dwarf_lineInfoParser_handleDefaultEntry(struct dwarf_lineInfoParser* self, uint8_t opCode);
void dwarf_lineInfoParser_handleSingeInstruction(struct dwarf_lineInfoParser* self, size_t* counter, uint8_t opCode);
void dwarf_lineInfoParser_handleSpecialOperation(struct dwarf_lineInfoParser* self, size_t* counter, uint64_t length,
                                                 uint8_t opCode);

#endif //CALLSTACKLIBRARY_DWARF_LINEINFOPARSER_H