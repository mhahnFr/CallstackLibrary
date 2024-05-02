/*
 * Callstack Library - Library creating human-readable call stacks.
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

#include <string.h>

#include "definitions.h"
#include "parser.h"
#include "vector_fileAttribute.h"

#include "../vector_uint8.h"
#include "../vector_pair_uint64.h"

static inline char* dwarf5_stringFromSection(uint64_t offset, uint64_t type, struct lcs_section debugLineStr, struct lcs_section debugStr) {
    char* toReturn = NULL;
    switch (type) {
        case DW_FORM_line_strp: toReturn = debugLineStr.content + offset; break;
        case DW_FORM_strp:      toReturn = debugStr.content + offset;     break;
    }
    return toReturn;
}

static inline char* dwarf5_readString(void* buffer, 
                                      size_t* counter,
                                      uint64_t type,
                                      bool bit64,
                                      struct lcs_section debugLineStr,
                                      struct lcs_section debugStr) {
    if (type == DW_FORM_string) {
        char* toReturn = (buffer + *counter);
        *counter += strlen(toReturn) + 1;
        return toReturn;
    }
    if (type != DW_FORM_line_strp && type != DW_FORM_strp && type != DW_FORM_strp_sup) {
        return NULL;
    }
    uint64_t offset;
    if (bit64) {
        offset = *((uint64_t*) (buffer + *counter));
        *counter += 8;
    } else {
        offset = *((uint32_t*) (buffer + *counter));
        *counter += 4;
    }
    return dwarf5_stringFromSection(offset, type, debugLineStr, debugStr);
}

static inline uint64_t dwarf5_readIndex(void* buffer, size_t* counter, uint64_t type) {
    uint64_t toReturn;
    switch (type) {
        case DW_FORM_data1:
            toReturn = *((uint8_t*) (buffer + (*counter)++));
            break;

        case DW_FORM_data2:
            toReturn = *((uint16_t*) (buffer + *counter));
            *counter += 2;

        case DW_FORM_udata:
            toReturn = getULEB128(buffer, counter);
            break;

        default: abort();
    }
    return toReturn;
}

static inline uint64_t dwarf5_readTimestamp(void* buffer, size_t* counter, uint64_t type) {
    uint64_t toReturn;
    switch (type) {
        case DW_FORM_udata:
            toReturn = getULEB128(buffer, counter);
            break;

        case DW_FORM_data4:
            toReturn = *((uint32_t*) (buffer + *counter));
            *counter += 4;
            break;

        case DW_FORM_data8:
            toReturn = *((uint64_t*) (buffer + *counter));
            *counter += 8;
            break;

        case DW_FORM_block: {
//            const uint64_t length = getULEB128(buffer, counter);
//            uint8_t buffer[length];
//            for (uint64_t i = 0; i < length; ++i) {
//                buffer[i] = buffer + *counter + i;
//            }
//            *counter += length;
            abort();
//            toReturn = <#buffer#>;
            break;
        }

        default: abort();
    }
    return toReturn;
}

static inline uint64_t dwarf5_readSize(void* buffer, size_t* counter, uint64_t type) {
    uint64_t toReturn;
    switch (type) {
        case DW_FORM_udata:
            toReturn = getULEB128(buffer, counter);
            break;

        case DW_FORM_data1:
            toReturn = *((uint8_t*) (buffer + (*counter)++));
            break;

        case DW_FORM_data2:
            toReturn = *((uint16_t*) (buffer + *counter));
            *counter += 2;
            break;

        case DW_FORM_data4:
            toReturn = *((uint32_t*) (buffer + *counter));
            *counter += 4;
            break;

        case DW_FORM_data8:
            toReturn = *((uint64_t*) (buffer + *counter));
            *counter += 8;
            break;

        default: abort();
    }
    return toReturn;
}

static inline uint8_t* dwarf5_readMD5(void* buffer, size_t* counter) {
    uint8_t* toReturn = buffer + *counter;
    *counter += 16;
    return toReturn;
}

static inline void dwarf5_consumeSome(void* buffer, size_t* counter, uint64_t type, bool bit64) {
    switch (type) {
        case DW_FORM_block: {
            const uint64_t length = getULEB128(buffer, counter);
            *counter += length;
            break;
        }

        case DW_FORM_block1: {
            uint8_t length = *((uint8_t*) (buffer + (*counter)++));
            *counter += length;
            break;
        }

        case DW_FORM_block2: {
            uint16_t length = *((uint16_t*) (buffer + *counter));
            *counter += 2 + length;
            break;
        }

        case DW_FORM_block4: {
            uint32_t length = *((uint32_t*) (buffer + *counter));
            *counter += 4 + length;
            break;
        }

        case DW_FORM_data1:  ++(*counter);   break;
        case DW_FORM_data2:  *counter += 2;  break;
        case DW_FORM_data4:  *counter += 4;  break;
        case DW_FORM_data8:  *counter += 8;  break;
        case DW_FORM_data16: *counter += 16; break;

        case DW_FORM_strp:
        case DW_FORM_string:
        case DW_FORM_line_strp:
            (void) dwarf5_readString(buffer, counter, type, bit64,
                                     (struct lcs_section) { .content = NULL, .size = 0 },
                                     (struct lcs_section) { .content = NULL, .size = 0 });
            break;

        case DW_FORM_sdata: getLEB128(buffer, counter);  break;
        case DW_FORM_udata: getULEB128(buffer, counter); break;

        case DW_FORM_sec_offset:
        case DW_FORM_flag:
        case DW_FORM_strx:
        case DW_FORM_strx1:
        case DW_FORM_strx2:
        case DW_FORM_strx3:
        case DW_FORM_strx4: abort();

    }
}

static inline vector_fileAttribute_t dwarf5_parseFileAttributes(void* buffer, size_t* counter, bool bit64, struct lcs_section debugLineStr, struct lcs_section debugStr) {
    const uint8_t entryFormatCount = *((uint8_t*) (buffer + (*counter)++));
    vector_pair_uint64_t entryFormats = vector_initializer;
    vector_pair_uint64_reserve(&entryFormats, entryFormatCount);
    for (uint8_t i = 0; i < entryFormatCount; ++i) {
        const uint64_t contentType = getULEB128(buffer, counter),
                          formCode = getULEB128(buffer, counter);
        vector_pair_uint64_push_back(&entryFormats, (pair_uint64_t) { contentType, formCode });
    }

    const uint64_t attributeCount = getULEB128(buffer, counter);
    vector_fileAttribute_t attributes = vector_initializer;
    vector_fileAttribute_reserve(&attributes, attributeCount);
    for (uint64_t i = 0; i < attributeCount; ++i) {
        struct fileAttribute attribute;
        bzero(&attribute, sizeof(struct fileAttribute));
        vector_iterate(pair_uint64_t, &entryFormats, {
            switch (element->first) {
                case DW_LNCT_path:
                    attribute.path = dwarf5_readString(buffer, counter, element->second, bit64, debugLineStr, debugStr);
                    break;

                case DW_LNCT_directory_index:
                    attribute.index = dwarf5_readIndex(buffer, counter, element->second);
                    break;

                case DW_LNCT_timestamp:
                    attribute.timestamp = dwarf5_readTimestamp(buffer, counter, element->second);
                    break;

                case DW_LNCT_size:
                    attribute.size = dwarf5_readSize(buffer, counter, element->second);
                    break;

                case DW_LNCT_MD5:
                    // TODO: Check
                    attribute.md5 = dwarf5_readMD5(buffer, counter); // Always DW_FROM_data16
                    break;

                default:
                    dwarf5_consumeSome(buffer, counter, element->second, bit64);
                    break; // Skip as defined by the paired value
            }
            vector_fileAttribute_push_back(&attributes, attribute);
        })
    }

    return attributes;
}

bool dwarf5_parseLineProgram(struct lcs_section debugLine,
                             struct lcs_section debugLineStr,
                             struct lcs_section debugStr,
                             size_t   counter,
                             uint64_t actualSize,
                             bool     bit64,
                             dwarf_line_callback cb, va_list args) {
    const uint8_t addressSize = *((uint8_t*) (debugLine.content + counter++));
    const uint8_t segmentSelectorSize = *((uint8_t*) (debugLine.content + counter++));

    uint64_t headerLength;
    if (bit64) {
        headerLength = *((uint64_t*) (debugLine.content + counter));
        counter += 8;
    } else {
        headerLength = *((uint32_t*) (debugLine.content + counter));
        counter += 4;
    }

    const uint8_t minimumInstructionLength = *((uint8_t*) (debugLine.content + counter++));
    const uint8_t maximumOperationsPerInstruction = *((uint8_t*) (debugLine.content + counter++));
    const bool    defaultIsStmt = *((uint8_t*) (debugLine.content + counter++));
    const int8_t  lineBase = *((int8_t*) (debugLine.content + counter++));
    const uint8_t lineRange = *((uint8_t*) (debugLine.content + counter++));
    const uint8_t opcodeBase = *((uint8_t*) (debugLine.content + counter++));

    vector_uint8_t stdOpcodeLengths;
    vector_uint8_create(&stdOpcodeLengths);
    vector_uint8_reserve(&stdOpcodeLengths, opcodeBase - 2);
    for (uint8_t i = 1; i < opcodeBase; ++i) {
        vector_uint8_push_back(&stdOpcodeLengths, *((uint8_t*) (debugLine.content + counter++)));
    }

    vector_fileAttribute_t directories = dwarf5_parseFileAttributes(debugLine.content, &counter, bit64, debugLineStr, debugStr),
                                 files = dwarf5_parseFileAttributes(debugLine.content, &counter, bit64, debugLineStr, debugStr);

    return false;
}
