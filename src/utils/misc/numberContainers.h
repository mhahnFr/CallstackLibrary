/*
* CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2025  mhahnFr
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

#ifndef utils_misc_numberContainers_h
#define utils_misc_numberContainers_h

#include <stdint.h>

#include <DC4C/optional.h>
#include <DC4C/pair.h>
#include <DC4C/vector.h>

typedef_optional_named(uint64, uint64_t);
typedef_pair_named(uint64, uint64_t, uint64_t);
typedef_vector_named(pair_uint64, struct pair_uint64);

#endif /* utils_misc_numberContainers_h */
