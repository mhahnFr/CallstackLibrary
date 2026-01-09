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

#include "containers_funcFile.h"
#include "objectFile.h"

typedef void (*machoParser_addFunction)(void* arg, pair_funcFile_t);

struct machoParser {
    struct symtab_command* command;
    const void* baseAddress;
    bool bytesSwapped, bit64;
    uintptr_t parsingOffset;
    machoParser_addFunction functionCallback;
    void* object;

// private:
    const char* stringTable;
    size_t entrySize;
    struct State {
        struct optional_function currentFunction;
        struct objectFile* currentObjectFile;
        const char* path, *sourceFilename;
    } parsingState;
};

struct machoParser machoParser_create(
    struct symtab_command* command, const void* baseAddress,
    uintptr_t parsingOffset, bool bytesSwapped, bool bit64,
    machoParser_addFunction functionCallback, void* object);

bool machoParser_parseSymbolTable(struct machoParser* self);
void machoParser_destroy(const struct machoParser* self);

#endif /* macho_parser_h */
