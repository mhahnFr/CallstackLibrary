/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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

#ifndef __lcs_functionInfo_functionInfo_h
#define __lcs_functionInfo_functionInfo_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents the gathered information for a function.
 *
 * @since v2.1
 */
struct functionInfo {
    /** The beginning address of the function in memory. */
    uintptr_t begin;
    /** The length of the function. May include padding. */
    size_t length;
    /** Whether the requested function was found.        */
    bool found;
};

/**
 * @brief Loads the information for the function of the given name.
 *
 * The runtime image of the given name is searched for the function. If the
 * function is not found in it all loaded runtime images are searched. If no
 * runtime image of the given name is loaded all runtime images are searched.
 *
 * @param functionName the name of the function to load its information
 * @param libraryName the name of the runtime image to search first
 * @return the gathered information for the desired function
 * @since v2.1
 */
struct functionInfo functionInfo_loadHint(const char* functionName, const char* libraryName);

/**
 * @brief Loads the information for the function of the given name.
 *
 * All loaded runtime images are searched for the function.
 *
 * @param functionName the name of the function to load its information
 * @return the gathered information for the desired function
 * @since v2.1
 */
static inline struct functionInfo functionInfo_load(const char* functionName) {
    return functionInfo_loadHint(functionName, NULL);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __lcs_functionInfo_functionInfo_h */
