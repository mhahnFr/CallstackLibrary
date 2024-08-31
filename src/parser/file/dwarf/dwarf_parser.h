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

/**
 * @brief This type represents the function called by `dwarf_parseLineProgram`.
 *
 * It takes a DWARF line info structure and the additionally passed arguments.
 */
typedef void (*dwarf_line_callback)(struct dwarf_lineInfo, void*);

/**
 * This structure represents a generified DWARF parser.
 */
struct dwarf_parser {
    /** The DWARF version.                                                              */
    uint16_t version;
    /** Whether the 64 Bit format is used.                                              */
    bool bit64;
    /** Whether the stmt value is `true` by default.                                    */
    bool defaultIsStmt;

    /** The base value for the op codes.                                                */
    uint8_t opCodeBase,
    /** The maximum amount of operations per instruction.                               */
            maximumOperationsPerInstruction,
    /** The line range.                                                                 */
            lineRange,
    /** The minimum instruction length.                                                 */
            minimumInstructionLength;
    /** The line base value.                                                            */
    int8_t lineBase;

    /** Vector with the argument count of the standard op codes.                        */
    vector_uint8_t stdOpcodeLengths;

    /** The `.debug_line` section.                                                      */
    struct lcs_section debugLine,
    /** The `.debug_str` section.                                                       */
                       debugStr,
    /** The `.debug_line_str` section.                                                  */
                       debugLineStr,
                       debugInfo,
                       debugAbbrev,
                       debugStrOffsets;

    /** The callback to be called when a line number table row is emitted.              */
    dwarf_line_callback cb;
    /** The payload for the DWARF line callback.                                        */
    void* args;

    char* compilationDirectory;

    /** The function to destroy the version dependent part of this parser.              */
    void                    (*destroy)    (struct dwarf_parser*);
    /** The function to parse the line number program's header for the current version. */
    bool                    (*parseHeader)(struct dwarf_parser*, size_t*);
    /** Creates the file reference for the given file index.                            */
    struct dwarf_sourceFile (*getFileName)(struct dwarf_parser*, uint64_t);

    union {
        /** The parser part for version 4. */
        struct dwarf4_parser v4;
        /** The parser part for version 5. */
        struct dwarf5_parser v5;
    } specific;
};

/**
 * @brief Parses the line program.
 *
 * Calls the callback with each emitted line table row and the given payload.
 *
 * @param debugLine the section corresponding to the .debug_line section
 * @param debugLineStr the section corresponding to the .debug_line_str section
 * @param debugStr the section corresponding to the .debug_str section
 * @param cb the line table row callback
 * @param args the payload to additionally pass to the callback function
 */
bool dwarf_parseLineProgram(struct lcs_section debugLine,
                            struct lcs_section debugLineStr,
                            struct lcs_section debugStr,
                            struct lcs_section debugInfo,
                            struct lcs_section debugAbbrev,
                            struct lcs_section debugStrOffsets,
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

char* dwarf_pathConcatenate(const char* string1, const char* string2);
uint64_t dwarf_parseInitialSize(void* buffer, size_t* counter, bool* bit64);

#endif /* dwarf_parser_h */
