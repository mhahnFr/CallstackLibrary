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

#ifndef dwarf_parser_h
#define dwarf_parser_h

#include <stdbool.h>
#include <stdint.h>

#include "dwarf_lineInfo.h"
#include "vector_uint8.h"

#include "v4/parser.h"
#include "v5/parser.h"

#include "../lcs_section.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This type represents the function called by `dwarf_parseLineProgram`.
 *
 * It takes a DWARF line info structure and the additionally passed arguments.
 */
typedef void (*dwarf_line_callback)(struct dwarf_lineInfo, void*);

struct dwarf_parser {
    uint16_t version;
    bool bit64;
    bool defaultIsStmt;

    uint8_t opCodeBase, maximumOperationsPerInstruction, lineRange, minimumInstructionLength;
    int8_t lineBase;

    vector_uint8_t stdOpcodeLengths;

    struct lcs_section debugLine,
    debugStr,
    debugLineStr;

    dwarf_line_callback cb;
    void* args;

    void                    (*destroy)    (struct dwarf_parser*);
    bool                    (*parseHeader)(struct dwarf_parser*, size_t*);
    struct dwarf_sourceFile (*getFileName)(struct dwarf_parser*, uint64_t);

    union {
        struct dwarf4_parser v4;
        struct dwarf5_parser v5;
    } specific;
};

bool dwarf_parseLineProgram(struct lcs_section debugLine,
                            struct lcs_section debugLineStr,
                            struct lcs_section debugStr,
                            dwarf_line_callback cb, void* args);

/**
 * @brief Reads an unsigned LEB128 integer from the given memory at the given position.
 *
 * The given memory position points to the first byte after the read number once this function returns.
 *
 * @param begin the memory pointer
 * @param counter the memory position
 * @return the deducted number
 */
uint64_t getULEB128(void* begin, size_t* counter);

/**
 * @brief Reads a signed LEB128 integer from the given memory at the given position.
 *
 * The given memory position points to the first byte after the read number once this function returns.
 *
 * @param begin the memory pointer
 * @param counter the memory position
 * @return the deducted number
 */
int64_t getLEB128(void* begin, size_t* counter);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* dwarf_parser_h */
