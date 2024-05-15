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

#ifndef dwarf_definitions_h
#define dwarf_definitions_h

#define DW_LNS_copy               0x01
#define DW_LNS_advance_pc         0x02
#define DW_LNS_advance_line       0x03
#define DW_LNS_set_file           0x04
#define DW_LNS_set_column         0x05
#define DW_LNS_negate_stmt        0x06
#define DW_LNS_set_basic_block    0x07
#define DW_LNS_const_add_pc       0x08
#define DW_LNS_fixed_advance_pc   0x09
#define DW_LNS_set_prologue_end   0x0a
#define DW_LNS_set_epilogue_begin 0x0b
#define DW_LNS_set_isa            0x0c

#define DW_LNE_end_sequence      0x01
#define DW_LNE_set_address       0x02
#define DW_LNE_set_discriminator 0x04

#define DW_FORM_addr       0x01
#define DW_FORM_block2     0x03
#define DW_FORM_block4     0x04
#define DW_FORM_data2      0x05
#define DW_FORM_data4      0x06
#define DW_FORM_data8      0x07
#define DW_FORM_string     0x08
#define DW_FORM_block      0x09
#define DW_FORM_block1     0x0a
#define DW_FORM_data1      0x0b
#define DW_FORM_flag       0x0c
#define DW_FORM_sdata      0x0d
#define DW_FORM_strp       0x0e
#define DW_FORM_udata      0x0f

#endif /* dwarf_definitions_h */
