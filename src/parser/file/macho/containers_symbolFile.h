/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2026  mhahnFr
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

#ifndef optional_pair_symbolFile_h
#define optional_pair_symbolFile_h

#include <DC4C/optional.h>
#include <DC4C/pair.h>
#include <DC4C/vector.h>

#include "objectFile.h"
#include "../symbol.h"

typedef_pair_named(symbolFile, struct symbol, struct objectFile *);
typedef_optional_named(symbolFile, struct pair_symbolFile);
typedef_vector_named(pairSymbolFile, pair_symbolFile_t);

#endif /* optional_pair_symbolFile_h */
