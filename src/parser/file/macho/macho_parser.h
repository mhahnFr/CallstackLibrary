/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2025  mhahnFr
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

#ifndef macho_parser_h
#define macho_parser_h

#include <stdarg.h>
#include <stdbool.h>
#include <mach-o/loader.h>

#include "objectFile.h"
#include "optional_pair_funcFile.h"

/**
 * @brief The function prototype for the callback called with a new object file object.
 *
 * Takes the object file object and the additionally passed arguments.
 */
typedef void (*macho_addObjectFile)(struct objectFile*, va_list);
/**
 * @brief The function prototype for the callback called with a new function / object file pair.
 *
 * Takes the function / object file pair and the additionally passed arguments.
 */
typedef void (*macho_addFunction)(pair_funcFile_t, va_list);

/**
 * @brief Parses the given Mach-O symbol table.
 *
 * At least one callback function needs to be provided (the other one may be
 * @c NULL) or both. If none is given, the given symbol table is not parsed and
 * @c false is returned.
 * <br><br>
 * The object file callback will be called when all information associated with
 * an object file is read.<br>
 * The function / object file callback is called for every symbol that is not
 * external.<br>
 * Both functions will be passed the additional arguments passed to this
 * function.
 *
 * @param command the Mach-O symbol table load command
 * @param baseAddress the start address of the Mach-O file the given symbol
 * table is in
 * @param offset the additional parsing offset
 * @param bytesSwapped whether the bytes need to be swapped to be in host byte
 * order
 * @param bit64 whether a 64 bit Mach-O file is parsed
 * @param objCb the object file callback function
 * @param funCb the function / object file pair callback function
 * @param ... the additional parameters to pass to the given functions
 * @return whether the symbol table was parsed successfully
 */
bool macho_parseSymtab(struct symtab_command* command, 
                       const void*            baseAddress,
                       uint64_t               offset,
                       bool                   bytesSwapped,
                       bool                   bit64,
                       macho_addObjectFile    objCb,
                       macho_addFunction      funCb,
                       ...);

#endif /* macho_parser_h */
