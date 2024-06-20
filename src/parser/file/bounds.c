/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023 - 2024  mhahnFr
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

#include "bounds.h"

const void* upper_bound(const void* const key, const void* const begin, const size_t count, const size_t size, int (*compare)(const void*, const void*)) {
    const void* it,
              * first = begin;
    size_t step,
           amount = count;

    while (amount > 0) {
        it = first;
        step = amount / 2;
        it += step * size;

        if (compare(key, it) >= 0) {
            it += size;
            first = it;
            amount -= step + 1;
        } else {
            amount = step;
        }
    }

    if (first < begin || first >= begin + size * count) {
        return NULL;
    }

    return first;
}
