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

#ifndef __lcs_regions_regions_h
#define __lcs_regions_regions_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Defines a memory region.
 *
 * Contains the beginning and ending pointers as well as the name of the binary
 * file to which the memory region belongs to.
 *
 * @since v2.2
 */
struct region {
    /** The beginning of the memory region.   */
    uintptr_t begin,
    /** The end of the memory region.         */
              end;

    /** The raw name of the binary file.      */
    const char* name,
    /** The relative name of the binary file. */
              * nameRelative;
};

/**
 * Defines an array of multiple region information.
 *
 * @since v2.2
 */
struct regionInfo {
    /** The array of memory region structures.               */
    struct region* regions;
    /** The amount of memory region structures in the array. */
    size_t amount;
};

/**
 * @brief Returns an array containing the memory region information structures
 * of all currently loaded runtime images.
 *
 * These memory regions represent the locations global storage is possibly
 * found in.<br>
 * The returned array must be destructed after use with the function
 * @code regions_destroyInfo(const struct regionInfo* info)@endcode.
 * <br><br>
 * According to @c callstack_autoClearCaches cache pointers are used.
 *
 * @return an array with global storage regions
 * @since v2.2
 */
struct regionInfo regions_getLoadedRegions(void);

/**
 * @brief Returns an array containing the thread-local memory region
 * information structures of all currently loaded runtime images.
 *
 * They represent the locations thread-local storage is found in for the
 * calling thread.<br>
 * The returned array must be destructed after use with the function
 * @code regions_destroyInfo(const struct regionInfo* info)@endcode.
 * <br><br>
 * According to @c callstack_autoClearCaches cache pointers are used.
 *
 * @note Since thread-local storage is generally initialized on demand, calling
 * this function may result in initializing all thread-local variables for the
 * calling thread.
 *
 * @return an array with thread-local storage memory regions
 * @since v2.2
 */
struct regionInfo regions_getTLSRegions(void);

/**
 * Destructs the given region information.
 *
 * @param info the region information to be destructed
 * @since v2.2
 */
void regions_destroyInfo(const struct regionInfo* info);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __lcs_regions_regions_h */
