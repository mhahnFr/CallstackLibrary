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

#include <mach-o/dyld.h>

#include "../cache/cache.h"

void cache_loaded_load(struct vector_boolString* cache) {
    const uint32_t size = _dyld_image_count();
    for (uint32_t i = 0; i < size; ++i) {
        vector_boolString_push_back(cache, (struct pair_boolString) {
            true,
            _dyld_get_image_name(i)
        });
    }
}
