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
#include "v4/definitions.h"
#include "v5/definitions.h"

// FIXME: Move up!!!
#include "v5/vector_pair_uint64.h"

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

char* dwarf_pathConcatenate(const char* string1, const char* string2) {
    const size_t len1 = strlen(string1),
                 len2 = strlen(string2);
    char* toReturn = malloc(len1 + len2 + 2);
    if (toReturn == NULL) {
        return NULL;
    }
    memcpy(toReturn, string1, len1);
    toReturn[len1] = '/';
    memcpy(toReturn + len1 + 1, string2, len2);
    toReturn[len1 + len2 + 1] = '\0';
    return toReturn;
}

/**
 * @brief This function parses the actual DWARF line program.
 *
 * The registered callback function is called for each line number table row that is emitted.
 *
 * @param self the DWARF parser object
 * @param counter the counter of already read bytes (offset)
 * @param actualSize the size of the line number program including the header, as read from the header
 * @return whether the parsing was successful
 */
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
                    // else fallthrough

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

                case DW_LNS_set_prologue_end:
                    if (self->version > 2) {
                        prologueEnd = true;
                        break;
                    }
                    // else fallthrough

                case DW_LNS_set_epilogue_begin:
                    if (self->version > 2) {
                        epilogueBegin = true;
                        break;
                    }
                    // else fallthrough

                case DW_LNS_set_isa:
                    if (self->version > 2) {
                        isa = getULEB128(self->debugLine.content, &counter);
                        break;
                    }
                    // else fallthrough

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
        }, self->debugLineStr, self->debugStr, self->debugInfo, self->debugAbbrev, self->debugStrOffsets, self->cb, self->args);
    }
    return true;
}

static inline vector_pair_uint64_t dwarf_getAbbreviationTable(struct lcs_section section,
                                                              uint64_t abbreviationCode,
                                                              uint64_t offset,
                                                              uint16_t version) {
    vector_pair_uint64_t toReturn = vector_initializer;
    size_t counter = (size_t) offset;

    uint64_t code;
    do {
        code = getULEB128(section.content, &counter);
        if (code == 0) continue;

        const uint64_t tag = getULEB128(section.content, &counter);
        const uint8_t children = *((uint8_t*) (section.content + counter++));
        (void) tag;
        (void) children;

        uint64_t name, form;
        do {
            name = getULEB128(section.content, &counter);
            form = getULEB128(section.content, &counter);
            if (version >= 5 && form == DW_FORM_implicit_const) {
                const uint64_t value = getLEB128(section.content, &counter);
                (void) value;
            }

            if (code == abbreviationCode && name != 0 && (version < 5 ? form != 0 : true)) {
                vector_pair_uint64_push_back(&toReturn, (pair_uint64_t) { name, form });
            }
        } while (name != 0 && (version < 5 ? form != 0 : true));
    } while (code != abbreviationCode && counter < (size_t) section.size);

    return toReturn;
}

uint64_t dwarf_parseInitialSize(void* buffer, size_t* counter, bool* bit64) {
    const uint32_t size = *((uint32_t*) (buffer + *counter));
    *counter += 4;

    uint64_t toReturn;
    if (size == 0xffffffff) {
        toReturn = *((uint64_t*) (buffer + *counter));
        *bit64 = true;
        *counter += 8;
    } else {
        toReturn = size;
        *bit64 = false;
    }
    return toReturn;
}

static inline bool dwarf_parseCompDir(struct dwarf_parser* self) {
    bool bit64;
    size_t counter = 0;
    const uint64_t size = dwarf_parseInitialSize(self->debugInfo.content, &counter, &bit64);
    const uint16_t version = *((uint16_t*) (self->debugInfo.content + counter));
    counter += 2;

    uint8_t addressSize;
    uint64_t abbrevOffset;
    if (version == 5) {
        const uint8_t unitType = *((uint8_t*) (self->debugInfo.content + counter++));
        addressSize = *((uint8_t*) (self->debugInfo.content + counter++));
        if (bit64) {
            abbrevOffset = *((uint64_t*) (self->debugInfo.content + counter));
            counter += 8;
        } else {
            abbrevOffset = *((uint32_t*) (self->debugInfo.content + counter));
            counter += 4;
        }
        switch (unitType) {
            case DW_UT_skeleton:
            case DW_UT_split_compile: counter += 8; break;

            case DW_UT_type:
            case DW_UT_split_type: counter += 8 + (bit64 ? 8 : 4); break;

            default: break;
        }
    } else {
        if (bit64) {
            abbrevOffset = *((uint64_t*) (self->debugInfo.content + counter));
            counter += 8;
        } else {
            abbrevOffset = *((uint32_t*) (self->debugInfo.content + counter));
            counter += 4;
        }
        addressSize = *((uint8_t*) (self->debugInfo.content + counter++));
    }
    (void) size;
    (void) addressSize;

    const uint64_t abbrevCode = getULEB128(self->debugInfo.content, &counter);
    const vector_pair_uint64_t abbrevs = dwarf_getAbbreviationTable(self->debugAbbrev, abbrevCode, abbrevOffset, version);
    vector_iterate(pair_uint64_t, &abbrevs, {
        if (element->first == DW_AT_comp_dir) {
            self->compilationDirectory = dwarf5_readString(self->debugInfo.content,
                                                           &counter,
                                                           element->second,
                                                           bit64,
                                                           self->debugLineStr,
                                                           self->debugStr,
                                                           self->debugStrOffsets);
            break;
        } else if (version >= 5 && element->second == DW_FORM_implicit_const) {
            continue;
        } else if (element->second == DW_FORM_indirect) {
            const uint64_t actualForm = getULEB128(self->debugInfo.content, &counter);
            if (!dwarf5_consumeSome(self->debugInfo.content, &counter, actualForm, bit64)) {
                break;
            }
        } else if (!dwarf5_consumeSome(self->debugInfo.content, &counter, element->second, bit64)) {
            break;
        }
    })
    vector_pair_uint64_destroy(&abbrevs);
    return self->compilationDirectory != NULL;
}

bool dwarf_parseLineProgram(struct lcs_section debugLine,
                            struct lcs_section debugLineStr,
                            struct lcs_section debugStr,
                            struct lcs_section debugInfo,
                            struct lcs_section debugAbbrev,
                            struct lcs_section debugStrOffsets,
                            dwarf_line_callback cb, void* args) {
    bool bit64;
    size_t counter = 0;
    const uint64_t size = dwarf_parseInitialSize(debugLine.content, &counter, &bit64);
    const uint16_t version = *((uint16_t*) (debugLine.content + counter));
    counter += 2;
    
    struct dwarf_parser parser = {
        .version = version,
        .bit64 = bit64,
        .debugLine = debugLine,
        .debugStr = debugStr,
        .debugLineStr = debugLineStr,
        .debugInfo = debugInfo,
        .debugAbbrev = debugAbbrev,
        .debugStrOffsets = debugStrOffsets,
        .cb = cb,
        .args = args,
        .stdOpcodeLengths = vector_initializer,
        .compilationDirectory = NULL
    };
    if (!dwarf_parseCompDir(&parser)) {
        return false;
    }
    switch (version) {
        case 2:
        case 3:
        case 4:
            dwarf4_parser_create(&parser);
            break;

        case 5: dwarf5_parser_create(&parser); break;

        default: return false;
    }

    const bool toReturn = dwarf_parser_parse(&parser, counter, size);
    vector_uint8_destroy(&parser.stdOpcodeLengths);
    parser.destroy(&parser);
    return toReturn;
}
