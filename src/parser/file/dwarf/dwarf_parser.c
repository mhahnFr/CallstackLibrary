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

#include "dwarf_parser.h"

#include <misc/numberContainers.h>

#include "leb128.h"
#include "v4/definitions.h"
#include "v5/definitions.h"

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
 * The registered callback function is called for each line number table row
 * that is emitted.
 *
 * @param self the DWARF parser object
 * @param counter the counter of already read bytes (offset)
 * @param actualSize the size of the line number program including the header,
 * as read from the header
 * @return whether the parsing was successful
 */
static inline bool dwarf_parser_parse(struct dwarf_parser* self, size_t counter, const size_t actualSize) {
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
        const uint8_t opCode = *(uint8_t*) (self->debugLine.content + counter++);
        if (opCode == 0) {
            const uint64_t length = getULEB128(self->debugLine.content, &counter);
            const uint8_t  actualOpCode = *(uint8_t*) (self->debugLine.content + counter++);
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
                    const size_t newAddress = *(size_t*) (self->debugLine.content + counter);
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
                        vector_push_back(&self->specific.v4.fileNames, ((struct dwarf_fileNameEntry) {
                            fileName, dirIndex, timeStamp, size
                        }));
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
                        address += adjustedOpCode / self->lineRange * self->minimumInstructionLength;
                    }
                    break;
                }

                case DW_LNS_fixed_advance_pc: {
                    opIndex = 0;
                    address += *(uint16_t*) (self->debugLine.content + counter);
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
            const uint8_t adjustedOpCode = opCode - self->opCodeBase;
            if (self->version > 3) {
                const uint8_t operationAdvance = adjustedOpCode / self->lineRange;

                address += self->minimumInstructionLength * ((opIndex + operationAdvance) / self->maximumOperationsPerInstruction);
                opIndex  = (opIndex + operationAdvance) % self->maximumOperationsPerInstruction;
            } else {
                address += adjustedOpCode / self->lineRange * self->minimumInstructionLength;
            }
            line += self->lineBase + adjustedOpCode % self->lineRange;

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

/**
 * @brief Parses the abbreviation table and returns the description found for
 * the given abbreviation code.
 *
 * Implicitly constants of DWARF in version 5 are handled but discarded.
 *
 * @note This function does not check any parameters for validity.
 *
 * @param section the debug abbreviation section
 * @param abbreviationCode the abbreviation code whose description to load
 * @param offset the offset into the section
 * @param version the DWARF version to be used
 * @return a vector with the type descriptions
 */
static inline vector_pair_uint64_t dwarf_getAbbreviationTable(const struct lcs_section section,
                                                              const uint64_t abbreviationCode,
                                                              const uint64_t offset,
                                                              const uint16_t version) {
    vector_pair_uint64_t toReturn = vector_initializer;
    size_t counter = (size_t) offset;

    uint64_t code;
    do {
        code = getULEB128(section.content, &counter);
        if (code == 0) continue;

        const uint64_t tag = getULEB128(section.content, &counter);
        const uint8_t children = *(uint8_t*) (section.content + counter++);
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
                vector_push_back(&toReturn, ((pair_uint64_t) { name, form }));
            }
        } while (name != 0 && (version < 5 ? form != 0 : true));
    } while (code != abbreviationCode && counter < (size_t) section.size);

    return toReturn;
}

uint64_t dwarf_parseInitialSize(void* buffer, size_t* counter, bool* bit64) {
    const uint32_t size = *(uint32_t*) (buffer + *counter);
    *counter += 4;

    uint64_t toReturn;
    if (size == 0xffffffff) {
        toReturn = *(uint64_t*) (buffer + *counter);
        *bit64 = true;
        *counter += 8;
    } else {
        toReturn = size;
        *bit64 = false;
    }
    return toReturn;
}

bool dwarf_consumeSome(const struct dwarf_parser* self, void* buffer, size_t* counter, const uint64_t type) {
    switch (type) {
        case DW_FORM_block: {
            const uint64_t length = getULEB128(buffer, counter);
            *counter += length;
            break;
        }

        case DW_FORM_block1: {
            const uint8_t length = *(uint8_t*) (buffer + (*counter)++);
            *counter += length;
            break;
        }

        case DW_FORM_block2: {
            const uint16_t length = *(uint16_t*) (buffer + *counter);
            *counter += 2 + length;
            break;
        }

        case DW_FORM_block4: {
            const uint32_t length = *(uint32_t*) (buffer + *counter);
            *counter += 4 + length;
            break;
        }

        case DW_FORM_flag:
        case DW_FORM_strx1:
        case DW_FORM_data1:  ++*counter;     break;
        case DW_FORM_strx2:
        case DW_FORM_data2:  *counter += 2;  break;
        case DW_FORM_strx3:  *counter += 3;  break;
        case DW_FORM_strx4:
        case DW_FORM_data4:  *counter += 4;  break;
        case DW_FORM_data8:  *counter += 8;  break;
        case DW_FORM_data16: *counter += 16; break;

        case DW_FORM_strp:
        case DW_FORM_string:
        case DW_FORM_line_strp:
            (void) dwarf_readString(self, buffer, counter, type);
            break;

        case DW_FORM_sdata: getLEB128(buffer, counter);  break;

        case DW_FORM_strx:
        case DW_FORM_udata: getULEB128(buffer, counter); break;

        case DW_FORM_sec_offset: *counter += self->bit64 ? 8 : 4; break;

        default: return false;
    }
    return true;
}

/**
 * Calculates a string pointer into one of the given sections.
 *
 * @param offset the string offset
 * @param type the type of string to load
 * @param debugLineStr the section corresponding to the @c .debug_line_str section
 * @param debugStr the section corresponding to the @c .debug_str section
 * @return a pointer to the string in either section or @c NULL if the given
 * type specifies neither section
 */
static inline char* dwarf_stringFromSection(const uint64_t offset,
                                            const uint64_t type,
                                            const struct lcs_section debugLineStr,
                                            const struct lcs_section debugStr) {
    char* toReturn = NULL;
    switch (type) {
        case DW_FORM_line_strp: toReturn = debugLineStr.content + offset; break;
        case DW_FORM_strp:      toReturn = debugStr.content + offset;     break;

        default: break;
    }
    return toReturn;
}

/**
 * @brief Loads the offset into the debug string section for the given index in
 * the given debug string offsets section.
 *
 * The section must not be empty, the index though is range checked.
 *
 * @param index the index of the offset
 * @param debugStrOffsets the debug string offsets section
 * @param offset the optional offset into the debug string offsets table
 * @return the optionally deducted string table offset
 */
static inline optional_uint64_t dwarf_loadStringOffset(const uint64_t index,
                                                       const struct lcs_section debugStrOffsets,
                                                       const optional_uint64_t offset) {
    bool bit64;
    size_t counter = 0;
    const uint64_t size = dwarf_parseInitialSize(debugStrOffsets.content, &counter, &bit64);
    if (bit64) {
        if (index >= size / 8) {
            return (optional_uint64_t) { .has_value = false };
        }
        if (offset.has_value) {
            return (optional_uint64_t) { true, ((uint64_t*) (debugStrOffsets.content + offset.value))[index] };
        }
        return (optional_uint64_t) { true, ((uint64_t*) (debugStrOffsets.content + counter))[index] };
    } else if (index >= size / 4) {
        return (optional_uint64_t) { .has_value = false };
    }
    if (offset.has_value) {
        return (optional_uint64_t) { true, ((uint32_t*) (debugStrOffsets.content + offset.value))[index] };
    }
    return (optional_uint64_t) { true, ((uint32_t*) (debugStrOffsets.content + counter))[index] };
}

char* dwarf_readString(const struct dwarf_parser* self, void* buffer, size_t* counter, uint64_t type) {
    if (type == DW_FORM_string) {
        char* toReturn = buffer + *counter;
        *counter += strlen(toReturn) + 1;
        return toReturn;
    }
    if (type != DW_FORM_line_strp && type != DW_FORM_strp && type != DW_FORM_strp_sup
        && type != DW_FORM_strx && type != DW_FORM_strx1 && type != DW_FORM_strx2
        && type != DW_FORM_strx3 && type != DW_FORM_strx4) {
        return NULL;
    }
    uint64_t offset;
    if (type == DW_FORM_strp || type == DW_FORM_line_strp || type == DW_FORM_strp_sup) {
        if (self->bit64) {
            offset = *(uint64_t*) (buffer + *counter);
            *counter += 8;
        } else {
            offset = *(uint32_t*) (buffer + *counter);
            *counter += 4;
        }
    } else {
        uint64_t index;
        switch (type) {
            case DW_FORM_strx:  index = getULEB128(buffer, counter);       break;
            case DW_FORM_strx1: index = *(uint8_t*) (buffer + *counter++); break;

            case DW_FORM_strx2:
                index = *(uint16_t*) (buffer + *counter);
                *counter += 2;
                break;

            case DW_FORM_strx3: {
                uint8_t bytes[3];
                bytes[0] = *(uint8_t*) (buffer + *counter++);
                bytes[1] = *(uint8_t*) (buffer + *counter++);
                bytes[2] = *(uint8_t*) (buffer + *counter++);

                index = bytes[0] + (bytes[1] << 8) + (bytes[2] << 16);
                break;
            }

            case DW_FORM_strx4:
                index = *(uint32_t*) (buffer + *counter);
                *counter += 4;
                break;

            default: return NULL;
        }
        type = DW_FORM_strp;
        const optional_uint64_t value = dwarf_loadStringOffset(index, self->debugStrOffsets, self->debugStrOffset);
        if (!value.has_value) {
            return NULL;
        }
        offset = value.value;
    }
    return dwarf_stringFromSection(offset, type, self->debugLineStr, self->debugStr);
}

/**
 * Parses the compilation directory.
 *
 * @param self the DWARF parser object
 * @return whether the parsing was successful
 */
static inline bool dwarf_parseCompDir(struct dwarf_parser* self) {
    bool bit64;
    size_t counter = 0;
    const uint64_t size = dwarf_parseInitialSize(self->debugInfo.content, &counter, &bit64);
    const uint16_t version = *(uint16_t*) (self->debugInfo.content + counter);
    counter += 2;

    uint8_t addressSize;
    uint64_t abbrevOffset;
    if (version == 5) {
        const uint8_t unitType = *(uint8_t*) (self->debugInfo.content + counter++);
        addressSize = *(uint8_t*) (self->debugInfo.content + counter++);
        if (bit64) {
            abbrevOffset = *(uint64_t*) (self->debugInfo.content + counter);
            counter += 8;
        } else {
            abbrevOffset = *(uint32_t*) (self->debugInfo.content + counter);
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
            abbrevOffset = *(uint64_t*) (self->debugInfo.content + counter);
            counter += 8;
        } else {
            abbrevOffset = *(uint32_t*) (self->debugInfo.content + counter);
            counter += 4;
        }
        addressSize = *(uint8_t*) (self->debugInfo.content + counter++);
    }
    (void) size;
    (void) addressSize;

    const uint64_t abbrevCode = getULEB128(self->debugInfo.content, &counter);
    const vector_pair_uint64_t abbrevs = dwarf_getAbbreviationTable(self->debugAbbrev, abbrevCode, abbrevOffset, version);
    vector_iterate(&abbrevs, {
        if (element->first == DW_AT_comp_dir) {
            self->compilationDirectory = dwarf_readString(self,
                                                          self->debugInfo.content,
                                                          &counter,
                                                          element->second);
            break;
        } else if (version >= 5 && element->first == DW_AT_str_offsets_base) {
            if (self->bit64) {
                self->debugStrOffset.value = *(uint64_t*) (self->debugInfo.content + counter);
                counter += 8;
            } else {
                self->debugStrOffset.value = *(uint32_t*) (self->debugInfo.content + counter);
                counter += 4;
            }
            self->debugStrOffset.has_value = true;
        } else if (version >= 5 && element->second == DW_FORM_implicit_const) {
            continue;
        } else if (element->second == DW_FORM_indirect) {
            const uint64_t actualForm = getULEB128(self->debugInfo.content, &counter);
            if (!dwarf_consumeSome(self, self->debugInfo.content, &counter, actualForm)) {
                break;
            }
        } else if (!dwarf_consumeSome(self, self->debugInfo.content, &counter, element->second)) {
            break;
        }
    });
    vector_destroy(&abbrevs);
    return self->compilationDirectory != NULL;
}

bool dwarf_parseLineProgram(const struct lcs_section debugLine,
                            const struct lcs_section debugLineStr,
                            const struct lcs_section debugStr,
                            const struct lcs_section debugInfo,
                            const struct lcs_section debugAbbrev,
                            const struct lcs_section debugStrOffsets,
                            const dwarf_line_callback cb, void* args) {
    bool bit64;
    size_t counter = 0;
    const uint64_t size = dwarf_parseInitialSize(debugLine.content, &counter, &bit64);
    const uint16_t version = *(uint16_t*) (debugLine.content + counter);
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
        .compilationDirectory = NULL,
        .debugStrOffset = (optional_uint64_t) { .has_value = false },
    };
    if (!dwarf_parseCompDir(&parser)) {
        return false;
    }
    switch (version) {
        case 2:
        case 3:
        case 4: dwarf4_parser_create(&parser); break;
        case 5: dwarf5_parser_create(&parser); break;

        default: return false;
    }

    const bool toReturn = dwarf_parser_parse(&parser, counter, size);
    vector_destroy(&parser.stdOpcodeLengths);
    parser.destroy(&parser);
    return toReturn;
}
