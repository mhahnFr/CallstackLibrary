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

#ifndef dwarf_v5_definitions_h
#define dwarf_v5_definitions_h

#include "../v4/definitions.h"

#define DW_FORM_strx           0x1a
#define DW_FORM_strp_sup       0x1d
#define DW_FORM_data16         0x1e
#define DW_FORM_line_strp      0x1f
#define DW_FORM_implicit_const 0x21
#define DW_FORM_strx1          0x25
#define DW_FORM_strx2          0x26
#define DW_FORM_strx3          0x27
#define DW_FORM_strx4          0x28

#define DW_LNCT_path            0x1
#define DW_LNCT_directory_index 0x2
#define DW_LNCT_timestamp       0x3
#define DW_LNCT_size            0x4
#define DW_LNCT_MD5             0x5

#define DW_UT_compile       0x01
#define DW_UT_type          0x02
#define DW_UT_partial       0x03
#define DW_UT_skeleton      0x04
#define DW_UT_split_compile 0x05
#define DW_UT_split_type    0x06

#endif /* dwarf_v5_definitions_h */
