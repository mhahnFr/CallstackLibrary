/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2026  mhahnFr
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

#include <stdbool.h>
#include <mach-o/loader.h>

#include "containers_symbolFile.h"
#include "objectFile.h"

/**
 * Callback function prototype for adding a newly deducted symbol.
 */
typedef void (*machoParser_addSymbol)(void* arg, pair_symbolFile_t);

/**
 * Represents a Mach-O symbol table parser.
 */
struct machoParser {
    /** The symbol table load command to be handled.            */
    struct symtab_command* command;
    /** The start address of the Mach-O file.                   */
    const void* baseAddress;
    /** Whether to swap endianness while parsing.               */
    bool bytesSwapped,
    /** Whether to parse in 64-bit mode.                        */
         bit64;
    /** The offset to be used for the parsing.                  */
    uintptr_t parsingOffset;
    /** The callback called once a symbol has been deducted.    */
    machoParser_addSymbol symbolCallback;
    /** The payload to pass on to the symbol callback function. */
    void* object;
    const char* fileName;

    /** Collection of the private member variables.             */
    struct {
        /** The beginning of the string table. */
        const char* stringTable;
        /** The size of an entry.              */
        size_t entrySize;
        /** The parsing state.                 */
        struct State {
            /** The currently handled symbol.      */
            struct optional_symbol currentSymbol;
            /** The currently handled object file. */
            struct objectFile* currentObjectFile;
            /** The path of the object file.       */
            const char* path,
            /** The name of the source file.       */
                      * sourceFilename;
        } parsingState;
    } private;
};

/**
 * Constructs a Mach-O symbol table parser using the given information.
 *
 * @param command the symbol table load command to handle
 * @param baseAddress the base address of the Mach-O file
 * @param parsingOffset the offset to be used while parsing
 * @param bytesSwapped whether to swap endianness
 * @param bit64 whether to parse in 64-bit mode
 * @param symbolCallback the callback to be called once a symbol has been
 * deducted
 * @param object the payload to be passed to the callback
 * @return the newly constructed Mach-O symbol table parser
 */
struct machoParser machoParser_create(
    struct symtab_command* command, const void* baseAddress,
    uintptr_t parsingOffset, bool bytesSwapped, bool bit64,
    machoParser_addSymbol symbolCallback, void* object, const char* fileName);

/**
 * Parses the symbol table the given Mach-O parser represents.
 *
 * @param self the Mach-O symbol table parser
 * @return whether the parsing was successful
 */
void machoParser_parseSymbolTable(struct machoParser* self);

/**
 * Destroys the given symbol table parser.
 *
 * @param self the Mach-O symbol table parser
 */
void machoParser_destroy(const struct machoParser* self);

#endif /* macho_parser_h */
