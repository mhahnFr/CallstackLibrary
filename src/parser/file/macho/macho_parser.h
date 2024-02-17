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

#include <mach-o/fat.h>
#include <mach-o/loader.h>

#include "function.h"
#include "objectFile.h"
#include "OptionalFuncFilePair.h"

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
 * At least one callback function needs to be provided (the other one 
 * may be `NULL`) or both. If none ise passed, the given symbol table
 * is not parsed and `false` is returned.
 *
 * The object file callback is called when all information associated
 * with an object file is read.
 * The function / object file callback is called for every symbol that
 * is not external.
 * Both functions are passed the additional arguments passed to this function.
 *
 * @param command the Mach-O symbol table load command
 * @param baseAddress the start address of the Mach-O file the given symbol table is in
 * @param offset the additional parsing offset
 * @param bytesSwapped whether the bytes need to be swapped to be in host byte order
 * @param bit64 whether a 64 bit Mach-O file is parsed
 * @param objCb the object file calback function
 * @param funCb the function / object file pair callback function
 * @return whether the symbol table was parsed successfully
 */
bool macho_parseSymtab(struct symtab_command* command, 
                       void*                  baseAddress,
                       uint64_t               offset,
                       bool                   bytesSwapped,
                       bool                   bit64,
                       macho_addObjectFile    objCb,
                       macho_addFunction      funCb,
                       ...);

/**
 * Extracts the appropriate Mach-O slice in the given fat archive.
 *
 * @param fatHeader the header of the fat archive
 * @param bitsReversed whether the bytes need to be reversed to match the host byte order
 * @param fileName the name of the represented Mach-O file
 * @return the slice the system would load or `NULL` if no appropriate slice is found
 */
void* macho_parseFat(struct fat_header* fatHeader, bool bitsReversed, const char* fileName);

#endif /* macho_parser_h */
