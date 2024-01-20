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

#include "macho_parser.h"

/*
 Format of the MachO debug symbols:
 
  SO: \0
  SO: <path>
  SO: <source_file_name>
 OSO: <full_object_path> <last_modified_time>
 ... <Symbols> ...
  SO: \0
 
 BNSYM: <function address>
   FUN: <linked name> <address>
   FUN: \0 <function's length>
 ENSYM: <function address>
 */

bool macho_parseSymtab(struct symtab_command* command,
                       void*                  baseAddress,
                       bool                   bytesSwapped,
                       macho_addObjectFile    objCb,
                       macho_addFunction      funCb) {
    // TODO: Implement
    
    (void) command;
    (void) baseAddress;
    (void) bytesSwapped;
    (void) objCb;
    (void) funCb;
    
    return true;
}

bool macho_parseSymtab64(struct symtab_command* command,
                         void*                  baseAddress,
                         bool                   bytesSwapped,
                         macho_addObjectFile    objCb,
                         macho_addFunction      funCb) {
    // TODO: Implement
    
    (void) command;
    (void) baseAddress;
    (void) bytesSwapped;
    (void) objCb;
    (void) funCb;
    
    return true;
}
