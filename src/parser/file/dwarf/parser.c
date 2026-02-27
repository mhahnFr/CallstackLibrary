/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2026  mhahnFr
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
#include "lineInfo/parser.h"

#include <misc/numberContainers.h>

#include "leb128.h"
#include "../dc4c_exceptions.h"
#include "../exception.h"
#include "v4/definitions.h"
#include "v5/definitions.h"

char* dwarf_pathConcatenate(const char* string1, const char* string2) {
    const size_t len1 = strlen(string1),
                 len2 = strlen(string2);
    char* toReturn = BFE_ALLOC_MSG(len1 + len2 + 2, "Failed to concatenate string");
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
static inline void dwarf_parser_parse(struct dwarf_parser* self, size_t counter, const size_t actualSize) {
    self->parseHeader(self, &counter);
    struct dwarf_lineInfoParser parser = dwarf_lineInfoParser_initializer(self->defaultIsStmt, self);
    while (counter - (self->bit64 ? 12 : 4) < actualSize) {
        const uint8_t opCode = *(uint8_t*) (self->debugLine.content + counter++);
        if (opCode == 0) {
            const uint64_t length = getULEB128(self->debugLine.content, &counter);
            dwarf_lineInfoParser_handleSpecialOperation(&parser, &counter, length, *(uint8_t*) (self->debugLine.content + counter++));
        } else if (opCode < self->opCodeBase) {
            dwarf_lineInfoParser_handleSingeInstruction(&parser, &counter, opCode);
        } else {
            dwarf_lineInfoParser_handleDefaultEntry(&parser, opCode);
        }
    }
    
    if (counter < self->debugLine.size - 2 - (self->bit64 ? 12 : 4)) {
        dwarf_parseLineProgram((struct lcs_section) {
            self->debugLine.content + counter,
            self->debugLine.size - 2 - (self->bit64 ? 12 : 4) - counter
        }, self->debugLineStr, self->debugStr, self->debugInfo, self->debugAbbrev, self->debugStrOffsets, self->cb, self->args);
    }
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
                TRY({
                    vector_push_back_throw(&toReturn, ((pair_uint64_t) { name, form }));
                }, CATCH_ALL(_, {
                    vector_destroy(&toReturn);
                    RETHROW;
                }))
            }
        } while (name != 0 && (version < 5 ? form != 0 : true));
    } while (code != abbreviationCode && counter < (size_t) section.size);

    return toReturn;
}

uint64_t dwarf_parseInitialSize(const void* buffer, size_t* counter, bool* bit64) {
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

void dwarf_consumeSome(const struct dwarf_parser* self, const void* buffer, size_t* counter, const uint64_t type) {
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

        default: BFE_THROW_MSG(unsupported, "Consuming unsupported DWARF data format");
    }
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
static inline const char* dwarf_stringFromSection(const uint64_t offset,
                                                  const uint64_t type,
                                                  const struct lcs_section debugLineStr,
                                                  const struct lcs_section debugStr) {
    const char* toReturn = NULL;
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
static inline uint64_t dwarf_loadStringOffset(const uint64_t index,
                                                       const struct lcs_section debugStrOffsets,
                                                       const optional_uint64_t offset) {
    bool bit64;
    size_t counter = 0;
    const uint64_t size = dwarf_parseInitialSize(debugStrOffsets.content, &counter, &bit64);
    if (bit64) {
        if (index >= size / 8) {
            BFE_THROW_MSG(invalid, "Invalid string index given");
        }
        return ((uint64_t*) (debugStrOffsets.content + (offset.has_value ? offset.value : counter)))[index];
    }
    if (index >= size / 4) {
        BFE_THROW_MSG(invalid, "Invalid string index given");
    }
    return ((uint32_t*) (debugStrOffsets.content + (offset.has_value ? offset.value : counter)))[index];
}

const char* dwarf_readString(const struct dwarf_parser* self, const void* buffer, size_t* counter, uint64_t type) {
    if (type == DW_FORM_string) {
        const char* toReturn = buffer + *counter;
        *counter += strlen(toReturn) + 1;
        return toReturn;
    }
    if (type != DW_FORM_line_strp && type != DW_FORM_strp && type != DW_FORM_strp_sup
        && type != DW_FORM_strx && type != DW_FORM_strx1 && type != DW_FORM_strx2
        && type != DW_FORM_strx3 && type != DW_FORM_strx4) {
        BFE_THROW_MSG(unsupported, "Unsupported string type to be parsed");
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

            default: BFE_THROW_MSG(unsupported, "Unsupported string type to be parsed");
        }
        type = DW_FORM_strp;
        offset = dwarf_loadStringOffset(index, self->debugStrOffsets, self->debugStrOffset);
    }
    return dwarf_stringFromSection(offset, type, self->debugLineStr, self->debugStr);
}

/**
 * Parses the compilation directory.
 *
 * @param self the DWARF parser object
 * @return whether the parsing was successful
 */
static inline const char* dwarf_parseCompDir(struct dwarf_parser* self) {
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
    TRY({
        vector_iterate(&abbrevs, {
            if (element->first == DW_AT_comp_dir) {
                const char* toReturn = dwarf_readString(self, self->debugInfo.content, &counter, element->second);
                vector_destroy(&abbrevs);
                TC_RETURN toReturn;
            }
            if (version >= 5 && element->first == DW_AT_str_offsets_base) {
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
                dwarf_consumeSome(self, self->debugInfo.content, &counter, actualForm);
            } else {
                dwarf_consumeSome(self, self->debugInfo.content, &counter, element->second);
            }
        });
        BFE_THROW_MSG(failed, "Failed to parse DW_AT_comp_dir");
    }, CATCH_ALL(_, {
        vector_destroy(&abbrevs);
        RETHROW;
    }))
}

void dwarf_parseLineProgram(const struct lcs_section debugLine,
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
    parser.compilationDirectory = dwarf_parseCompDir(&parser);
    switch (version) {
        case 2:
        case 3:
        case 4: dwarf4_parser_create(&parser); break;
        case 5: dwarf5_parser_create(&parser); break;

        default: BFE_THROW_MSG(unsupported, "Unsupported DWARF version");
    }
    TRY({
        dwarf_parser_parse(&parser, counter, size);
    }, CATCH_ALL(_, {
        vector_destroy(&parser.stdOpcodeLengths);
        parser.destroy(&parser);
        RETHROW;
    }))
    vector_destroy(&parser.stdOpcodeLengths);
    parser.destroy(&parser);
}