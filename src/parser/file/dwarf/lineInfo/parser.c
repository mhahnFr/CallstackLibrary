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

#include "parser.h"
#include "../parser.h"

#include "../definitions.h"
#include "../leb128.h"
#include "../v3/definitions.h"
#include "../v4/definitions.h"

void dwarf_lineInfoParser_handleDefaultEntry(struct dwarf_lineInfoParser* self, const uint8_t opCode) {
    const uint8_t adjustedOpCode = opCode - self->parser->opCodeBase;
    if (self->parser->version > 3) {
        const uint8_t operationAdvance = adjustedOpCode / self->parser->lineRange;

        self->address += self->parser->minimumInstructionLength
                         * ((self->opIndex + operationAdvance) / self->parser->maximumOperationsPerInstruction);
        self->opIndex  = (self->opIndex + operationAdvance) % self->parser->maximumOperationsPerInstruction;
    } else {
        self->address += adjustedOpCode / self->parser->lineRange * self->parser->minimumInstructionLength;
    }
    self->line += self->parser->lineBase + adjustedOpCode % self->parser->lineRange;

    self->parser->cb((struct dwarf_lineInfo) {
        self->address, self->line, self->column, self->isa, self->discriminator,
        self->parser->getFileName(self->parser, self->file),
        self->isStmt, self->basicBlock, self->endSequence, self->prologueEnd, self->epilogueBegin
    }, self->parser->args);

    self->basicBlock    = false;
    self->prologueEnd   = false;
    self->epilogueBegin = false;
    self->discriminator = 0;
}

static inline void dwarf_lineInfoParser_commandCopy(struct dwarf_lineInfoParser* self) {
    self->parser->cb((struct dwarf_lineInfo) {
        self->address, self->line, self->column, self->isa, self->discriminator,
        self->parser->getFileName(self->parser, self->file),
        self->isStmt, self->basicBlock, self->endSequence, self->prologueEnd, self->epilogueBegin
    }, self->parser->args);

    self->discriminator = 0;
    self->basicBlock = self->prologueEnd = self->epilogueBegin = false;
}

static inline void dwarf_lineInfoParser_commandAdvance(struct dwarf_lineInfoParser* self, size_t* counter) {
    const uint64_t operationAdvance = getULEB128(self->parser->debugLine.content, counter);
    if (self->parser->version > 3) {
        self->address += self->parser->minimumInstructionLength
                         * ((self->opIndex + operationAdvance) / self->parser->maximumOperationsPerInstruction);
        self->opIndex = (self->opIndex + operationAdvance) % self->parser->maximumOperationsPerInstruction;
    } else {
        self->address += self->parser->minimumInstructionLength * operationAdvance;
    }
}

static inline void dwarf_lineInfoParser_commandAdd(struct dwarf_lineInfoParser* self) {
    const uint8_t adjustedOpCode = 255 - self->parser->opCodeBase;
    if (self->parser->version > 3) {
        const uint8_t operationAdvance = adjustedOpCode / self->parser->lineRange;

        self->address += self->parser->minimumInstructionLength
                         * ((self->opIndex + operationAdvance) / self->parser->maximumOperationsPerInstruction);
        self->opIndex  = (self->opIndex + operationAdvance) % self->parser->maximumOperationsPerInstruction;
    } else {
        self->address += adjustedOpCode / self->parser->lineRange * self->parser->minimumInstructionLength;
    }
}

static inline void dwarf_lineInfoParser_commandOthers(struct dwarf_lineInfoParser* self, size_t* counter,
                                                      const uint8_t opCode) {
    if (self->parser->version > 2) switch (opCode) {
        case DW_LNS_set_prologue_end:   self->prologueEnd = true;                                         return;
        case DW_LNS_set_epilogue_begin: self->epilogueBegin = true;                                       return;
        case DW_LNS_set_isa:            self->isa = getULEB128(self->parser->debugLine.content, counter); return;

        default: break;
    }
    for (uint64_t i = 0; i < self->parser->stdOpcodeLengths.content[opCode - 1]; ++i) {
        getLEB128(self->parser->debugLine.content, counter);
    }
}

void dwarf_lineInfoParser_handleSingeInstruction(struct dwarf_lineInfoParser* self, size_t* counter,
                                                 const uint8_t opCode) {
    switch (opCode) {
        case DW_LNS_copy:            dwarf_lineInfoParser_commandCopy(self);                              break;
        case DW_LNS_advance_pc:      dwarf_lineInfoParser_commandAdvance(self, counter);                  break;
        case DW_LNS_advance_line:    self->line += getLEB128(self->parser->debugLine.content, counter);   break;
        case DW_LNS_set_file:        self->file = getULEB128(self->parser->debugLine.content, counter);   break;
        case DW_LNS_set_column:      self->column = getULEB128(self->parser->debugLine.content, counter); break;
        case DW_LNS_negate_stmt:     self->isStmt = !self->isStmt;                                        break;
        case DW_LNS_set_basic_block: self->basicBlock = true;                                             break;
        case DW_LNS_const_add_pc:    dwarf_lineInfoParser_commandAdd(self);                               break;

        case DW_LNS_fixed_advance_pc:
            self->opIndex = 0;
            self->address += *(uint16_t*) (self->parser->debugLine.content + *counter);
            *counter += 2;
            break;

        default: dwarf_lineInfoParser_commandOthers(self, counter, opCode); break;
    }
}

static inline void dwarf_lineInfoParser_handleEndSequence(struct dwarf_lineInfoParser* self) {
    self->endSequence = true;
    self->parser->cb((struct dwarf_lineInfo) {
        self->address, self->line, self->column, self->isa, self->discriminator,
        self->parser->getFileName(self->parser, self->file),
        self->isStmt, self->basicBlock, self->endSequence, self->prologueEnd, self->epilogueBegin
    }, self->parser->args);

    self->address = self->opIndex = self->column = self->isa = self->discriminator = 0;
    self->basicBlock = self->endSequence = self->prologueEnd = self->epilogueBegin = false;
    self->file = self->line = 1;
    self->isStmt = self->parser->defaultIsStmt;
}

static inline void dwarf_lineInfoParser_handleFileDefinition(const struct dwarf_lineInfoParser* self, size_t* counter) {
    const char* fileName = self->parser->debugLine.content + *counter;
    *counter += strlen(fileName) + 1;
    const uint64_t  dirIndex = getULEB128(self->parser->debugLine.content, counter),
                   timeStamp = getULEB128(self->parser->debugLine.content, counter),
                        size = getULEB128(self->parser->debugLine.content, counter);
    if (self->parser->version < 5) {
        vector_push_back(&self->parser->specific.v4.fileNames, ((struct dwarf_fileNameEntry) {
            fileName, dirIndex, timeStamp, size
        }));
    }
}

static inline void dwarf_lineInfoParser_handleOtherSpecials(struct dwarf_lineInfoParser* self, size_t* counter,
                                                            const uint64_t length, const uint8_t opCode) {
    if (self->parser->version > 3 && opCode == DW_LNE_set_discriminator) {
        self->discriminator = getULEB128(self->parser->debugLine.content, counter);
        return;
    }
    *counter += length - 1;
}

void dwarf_lineInfoParser_handleSpecialOperation(struct dwarf_lineInfoParser* self, size_t* counter,
                                                 const uint64_t length, const uint8_t opCode) {
    switch (opCode) {
        case DW_LNE_end_sequence: dwarf_lineInfoParser_handleEndSequence(self);             break;
        case DW_LNE_define_file:  dwarf_lineInfoParser_handleFileDefinition(self, counter); break;

        case DW_LNE_set_address:
            self->address = *(size_t*) (self->parser->debugLine.content + *counter);
            *counter += sizeof(size_t);
            self->opIndex = 0;
            break;

        default: dwarf_lineInfoParser_handleOtherSpecials(self, counter, length, opCode); break;
    }
}