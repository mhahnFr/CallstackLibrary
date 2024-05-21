/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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

#include "dwarf_parser.h"
#include "v3/definitions.h"

uint64_t getULEB128(void* begin, size_t* counter) {
    uint64_t result = 0,
             shift  = 0;
    
    bool more = true;
    do {
        uint8_t b = *((uint8_t*) (begin + *counter));
        *counter += 1;
        result |= (b & 0x7f) << shift;
        shift += 7;
        if (b < 0x80) {
            more = false;
        }
    } while (more);
    return result;
}

int64_t getLEB128(void* begin, size_t* counter) {
    int64_t result = 0,
            shift  = 0;
    
    bool more = true;
    do {
        uint8_t b = *((uint8_t*) (begin + *counter));
        *counter += 1;
        result |= (b & 0x7f) << shift;
        shift += 7;
        if ((0x80 & b) == 0) {
            if (shift < 32 && (b & 0x40) != 0) {
                result |= ((uint64_t) ~0 << shift);
            }
            more = false;
        }
    } while (more);
    return result;
}

static inline bool dwarf_parser_parse(struct dwarf_parser* self, size_t counter, size_t actualSize) {
    if (!self->parseHeader(self, &counter)) {
        return false;
    }

    uint64_t address       = 0,
             opIndex       = 0,
             file          = 1,
             line          = 1,
             column        = 0,
             isa           = 0,
             discriminator = 0;

    bool isStmt         = self->defaultIsStmt,
          basicBlock    = false,
          endSequence   = false,
          prologueEnd   = false,
          epilogueBegin = false;
    
    while (counter - (self->bit64 ? 12 : 4) < actualSize) {
        const uint8_t opCode = *((uint8_t*) (self->debugLine.content + counter++));
        if (opCode == 0) {
            const uint64_t length = getULEB128(self->debugLine.content, &counter);
            const uint8_t  actualOpCode = *((uint8_t*) (self->debugLine.content + counter++));
            switch (actualOpCode) {
                case DW_LNE_end_sequence: {
                    endSequence = true;
                    self->cb((struct dwarf_lineInfo) {
                        address, line, column, isa, discriminator,
                        self->getFileName(self, file),
                        isStmt, basicBlock, endSequence, prologueEnd, epilogueBegin
                    }, self->args);
                    
                    address = opIndex = column = isa = discriminator = 0;
                    basicBlock = endSequence = prologueEnd = epilogueBegin = false;
                    file = line = 1;
                    isStmt = self->defaultIsStmt;
                    break;
                }
                    
                case DW_LNE_set_address: {
                    const size_t newAddress = *((size_t*) (self->debugLine.content + counter));
                    counter += sizeof(size_t);
                    address = newAddress;
                    opIndex = 0;
                    break;
                }
                    
                case DW_LNE_define_file: {
                    const char* fileName = self->debugLine.content + counter;
                    counter += strlen(fileName) + 1;
                    const uint64_t  dirIndex = getULEB128(self->debugLine.content, &counter),
                                   timeStamp = getULEB128(self->debugLine.content, &counter),
                                        size = getULEB128(self->debugLine.content, &counter);
                    if (self->version < 5) {
                        vector_dwarfFileEntry_push_back(&self->specific.v4.fileNames, (struct dwarf_fileNameEntry) {
                            fileName, dirIndex, timeStamp, size
                        });
                    }
                    break;
                }

                case DW_LNE_set_discriminator:
                    if (self->version > 3) {
                        discriminator = getULEB128(self->debugLine.content, &counter);
                        break;
                    }

                default: counter += length - 1; break;
            }
        } else if (opCode < self->opCodeBase) {
            switch (opCode) {
                case DW_LNS_copy: {
                    self->cb((struct dwarf_lineInfo) {
                        address, line, column, isa, discriminator,
                        self->getFileName(self, file),
                        isStmt, basicBlock, endSequence, prologueEnd, epilogueBegin
                    }, self->args);
                    
                    discriminator = 0;
                    basicBlock = prologueEnd = epilogueBegin = false;
                    break;
                }
                    
                case DW_LNS_advance_pc: {
                    const uint64_t operationAdvance = getULEB128(self->debugLine.content, &counter);
                    if (self->version > 3) {
                        address += self->minimumInstructionLength * ((opIndex + operationAdvance) / self->maximumOperationsPerInstruction);
                        opIndex = (opIndex + operationAdvance) % self->maximumOperationsPerInstruction;
                    } else {
                        address += self->minimumInstructionLength * operationAdvance;
                    }
                    break;
                }
                    
                case DW_LNS_advance_line:    line += getLEB128(self->debugLine.content, &counter);   break;
                case DW_LNS_set_file:        file = getULEB128(self->debugLine.content, &counter);   break;
                case DW_LNS_set_column:      column = getULEB128(self->debugLine.content, &counter); break;
                case DW_LNS_negate_stmt:     isStmt = !isStmt;                                       break;
                case DW_LNS_set_basic_block: basicBlock = true;                                      break;

                case DW_LNS_const_add_pc: {
                    const uint8_t adjustedOpCode = 255 - self->opCodeBase;
                    if (self->version > 3) {
                        const uint8_t operationAdvance = adjustedOpCode / self->lineRange;

                        address += self->minimumInstructionLength * ((opIndex + operationAdvance) / self->maximumOperationsPerInstruction);
                        opIndex  = (opIndex + operationAdvance) % self->maximumOperationsPerInstruction;
                    } else {
                        address += (adjustedOpCode / self->lineRange) * self->minimumInstructionLength;
                    }
                    break;
                }
                    
                case DW_LNS_fixed_advance_pc: {
                    opIndex = 0;
                    address += *((uint16_t*) (self->debugLine.content + counter));
                    counter += 2;
                    break;
                }
                    
                case DW_LNS_set_prologue_end:   prologueEnd = true;                                  break;
                case DW_LNS_set_epilogue_begin: epilogueBegin = true;                                break;
                case DW_LNS_set_isa:            isa = getULEB128(self->debugLine.content, &counter); break;

                default:
                    for (uint64_t i = 0; i < self->stdOpcodeLengths.content[opCode - 1]; ++i) {
                        getLEB128(self->debugLine.content, &counter);
                    }
                    break;
            }
        } else {
            uint8_t adjustedOpCode = opCode - self->opCodeBase;
            if (self->version > 3) {
                uint8_t operationAdvance = adjustedOpCode / self->lineRange;

                address += self->minimumInstructionLength * ((opIndex + operationAdvance) / self->maximumOperationsPerInstruction);
                opIndex  = (opIndex + operationAdvance) % self->maximumOperationsPerInstruction;
            } else {
                address += (adjustedOpCode / self->lineRange) * self->minimumInstructionLength;
            }
            line += self->lineBase + (adjustedOpCode % self->lineRange);

            self->cb((struct dwarf_lineInfo) {
                address, line, column, isa, discriminator,
                self->getFileName(self, file),
                isStmt, basicBlock, endSequence, prologueEnd, epilogueBegin
            }, self->args);
            
            basicBlock    = false;
            prologueEnd   = false;
            epilogueBegin = false;
            discriminator = 0;
        }
    }
    
    if (counter < self->debugLine.size - 2 - (self->bit64 ? 12 : 4)) {
        return dwarf_parseLineProgram((struct lcs_section) {
            self->debugLine.content + counter,
            self->debugLine.size - 2 - (self->bit64 ? 12 : 4) - counter
        }, self->debugLineStr, self->debugStr, self->cb, self->args);
    }
    return true;
}

bool dwarf_parseLineProgram(struct lcs_section debugLine,
                            struct lcs_section debugLineStr,
                            struct lcs_section debugStr,
                            dwarf_line_callback cb, void* args) {
    size_t counter = 0;
    
    const uint32_t size = *((uint32_t*) debugLine.content);
    counter += 4;
    
    bool     bit64;
    uint64_t actualSize;
    if (size == 0xffffffff) {
        actualSize = *((uint64_t*) (debugLine.content + counter));
        bit64      = true;
        counter += 8;
    } else {
        actualSize = size;
        bit64      = false;
    }
    const uint16_t version = *((uint16_t*) (debugLine.content + counter));
    counter += 2;
    
    struct dwarf_parser parser = {
        .version = version,
        .bit64 = bit64,
        .debugLine = debugLine,
        .debugStr = debugStr,
        .debugLineStr = debugLineStr,
        .cb = cb,
        .args = args,
        .stdOpcodeLengths = vector_initializer
    };
    switch (version) {
        case 3:
        case 4: dwarf4_parser_create(&parser); break;
        case 5: dwarf5_parser_create(&parser); break;

        default: return false;
    }

    const bool toReturn = dwarf_parser_parse(&parser, counter, actualSize);
    vector_uint8_destroy(&parser.stdOpcodeLengths);
    parser.destroy(&parser);
    return toReturn;
}
