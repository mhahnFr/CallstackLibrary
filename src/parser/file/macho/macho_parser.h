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

#ifndef macho_parser_h
#define macho_parser_h

#include <mach-o/loader.h>

#include "function.h"
#include "objectFile.h"

typedef void (*macho_addObjectFile)(struct objectFile*);
typedef void (*macho_addFunction)(struct function);

bool macho_parseSymtab(struct symtab_command* command, 
                       void*                  baseAddress,
                       bool                   bytesSwapped,
                       macho_addObjectFile    objCb,
                       macho_addFunction      funCb);

bool macho_parseSymtab64(struct symtab_command* command,
                         void*                  baseAddress,
                         bool                   bytesSwapped,
                         macho_addObjectFile    objCb,
                         macho_addFunction      funCb);

#endif /* macho_parser_h */
