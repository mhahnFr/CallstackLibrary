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

#ifndef loader_h
#define loader_h

#include <stdbool.h>

typedef bool (*loader_parser)(void*, void*);
typedef bool (*loader_parserExtended)(void*, const char*, size_t, void*);

union loader_parserFunction {
    loader_parser parseFunc;
    loader_parserExtended parseFuncExtended;
};

bool loader_loadFileAndExecute(const char* fileName, union loader_parserFunction func, bool extended, void* args);

#endif /* loader_h */
