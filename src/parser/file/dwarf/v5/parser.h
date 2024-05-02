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

#ifndef dwaf_v5_parser_h
#define dwaf_v5_parser_h

#include "../dwarf_parser.h"

bool dwarf5_parseLineProgram(struct lcs_section debugLine,
                             struct lcs_section debugLineStr,
                             struct lcs_section debugStr,
                             size_t   counter,
                             uint64_t actualSize,
                             bool     bit64,
                             dwarf_line_callback cb, va_list args);

#endif /* dwaf_v5_parser_h */
