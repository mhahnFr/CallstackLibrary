/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
 *
 * This file is part of the CallstackLibrary. This library is free software:
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef dwarf_parser_h
#define dwarf_parser_h

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "dwarf_lineInfo.h"

/**
 * @brief This type represents the function called by `dwarf_parseLineProgram`.
 *
 * It takes a DWARF line info structure and the additionally passed arguments.
 */
typedef void (*dwarf_line_callback)(struct dwarf_lineInfo, va_list);

/**
 * Parses the DWARF line program.
 *
 * @param begin the begin of the line program
 * @param cb the callback called when a line info has been deducted
 * @param args additional arguments that are passed to the callback function
 * @param sectionSize the total size of the section the line program is in
 * @return whether the line program was parsed successfully
 */
bool dwarf_parseLineProgram(void* begin, dwarf_line_callback cb, va_list args, uint64_t sectionSize);

#endif /* dwarf_parser_h */
