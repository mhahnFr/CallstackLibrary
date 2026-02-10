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

typedef void (*machoParser_addSymbol)(void* arg, pair_symbolFile_t);

struct machoParser {
    struct symtab_command* command;
    const void* baseAddress;
    bool bytesSwapped, bit64;
    uintptr_t parsingOffset;
    machoParser_addSymbol symbolCallback;
    void* object;

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

struct machoParser machoParser_create(
    struct symtab_command* command, const void* baseAddress,
    uintptr_t parsingOffset, bool bytesSwapped, bool bit64,
    machoParser_addSymbol symbolCallback, void* object);

bool machoParser_parseSymbolTable(struct machoParser* self);
void machoParser_destroy(const struct machoParser* self);

#endif /* macho_parser_h */
