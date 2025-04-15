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

#ifndef utils_misc_string_utils_h
#define utils_misc_string_utils_h

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/**
 * Copies the given string if it is not `NULL` and requested.
 *
 * @param string the string to maybe copy
 * @param copy whether to copy the string
 * @return the copy or the original string if @c copy is @c false or the given string was @c NULL
 */
static inline char* utils_maybeCopySave(const char* string, bool copy) {
    if (copy && string != NULL) {
        return strdup(string);
    }
    return (char*) string;
}

#endif /* utils_misc_string_utils_h */
