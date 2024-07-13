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

#ifndef bounds_h
#define bounds_h

#include <stddef.h>

/**
 * Returns the upper bound of the given key in the given array.
 *
 * @param key the key whose upper bound to be found
 * @param begin the start pointer of the array
 * @param count the number of elements in the array
 * @param size the size of an individual element
 * @param compare the comparasion function to be used to search
 * @return the upper bound value of the given key in the given array
 */
const void* upper_bound(const void* key,
                        const void* begin,
                        size_t      count,
                        size_t      size,
                        int (*compare)(const void*, const void*));

/**
 * Returns the lower bound of the given key in the given array.
 *
 * @param key the key whose lower bound to be found
 * @param begin the start pointer of the array
 * @param count the number of elements in the array
 * @param size the soze of an individual element
 * @param compare the comparasion function to be used to search
 * @return the lower bound value of the given key in the given array
 */
const void* lower_bound(const void* key,
                        const void* begin,
                        size_t      count,
                        size_t      size,
                        int (*compare)(const void*, const void*));

#endif /* bounds_h */
