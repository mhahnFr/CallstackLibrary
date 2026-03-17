/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2026  mhahnFr
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

#ifndef CALLSTACKLIBRARY_DC4C_EXCEPTIONS_H
#define CALLSTACKLIBRARY_DC4C_EXCEPTIONS_H

#include <DC4C/vector.h>

#include "exception.h"

/**
 * Invokes @c vector_reserve and throws an exception if it fails.
 *
 * @param ... the arguments to pass on
 */
#define vector_reserve_throw(...) do {                                           \
    if (!vector_reserve(__VA_ARGS__)) {                                          \
        BFE_THROW_MSG(failedAllocation, "Vector reserve allocation has failed"); \
    }                                                                            \
} while (0)

/**
 * Invokes @c vector_push_back and throws an exception if it fails.
 *
 * @param ... the arguments to pass on
 */
#define vector_push_back_throw(...) do {                                           \
    if (!vector_push_back(__VA_ARGS__)) {                                          \
        BFE_THROW_MSG(failedAllocation, "Vector push_back allocation has failed"); \
    }                                                                              \
} while (0)

/**
 * Invokes @c vector_insert and throws an exception if it fails.
 *
 * @param ... the arguments to pass on
 */
#define vector_insert_throw(...) do {                                           \
    if (!vector_insert(__VA_ARGS__)) {                                          \
        BFE_THROW_MSG(failedAllocation, "Vector insert allocation has failed"); \
    }                                                                           \
} while (0)

#endif //CALLSTACKLIBRARY_DC4C_EXCEPTIONS_H