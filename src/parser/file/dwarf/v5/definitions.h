/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
 *
 * This file is part of the CallstackLibrary.
 *
 * CallstackLibrary is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CallstackLibrary is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * CallstackLibrary, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef dwarf_v5_definitions_h
#define dwarf_v5_definitions_h

#define DW_FORM_string    0x08
#define DW_FORM_strp      0x0e
#define DW_FORM_line_strp 0x1f
#define DW_FORM_strp_sup  0x1d

#define DW_FORM_strx  0x1a
#define DW_FORM_strx1 0x25
#define DW_FORM_strx2 0x26
#define DW_FORM_strx3 0x27
#define DW_FORM_strx4 0x28

#define DW_FORM_data1  0x0b
#define DW_FORM_data2  0x05
#define DW_FORM_data4  0x06
#define DW_FORM_data8  0x07
#define DW_FORM_data16 0x1e

#define DW_FORM_udata 0x0f
#define DW_FORM_sdata 0x0d

#define DW_FORM_block  0x09
#define DW_FORM_block1 0x0a
#define DW_FORM_block2 0x03
#define DW_FORM_block4 0x04

#define DW_FORM_flag 0x0c

#define DW_FORM_sec_offset 0x17

#define DW_LNCT_path            0x1
#define DW_LNCT_directory_index 0x2
#define DW_LNCT_timestamp       0x3
#define DW_LNCT_size            0x4
#define DW_LNCT_MD5             0x5

#endif /* dwarf_v5_definitions_h */
