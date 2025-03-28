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

#include <stdlib.h>
#include <string.h>

#include <callstack_internals.h>
#include <regions/regions.h>

#include <DC4C/vector.h>

#include "../dlMapper/dlMapper.h"

typedef_vector_light_named(region, struct region);

#define maybeRun(b, func, ...) ({ (b) ? (func)(__VA_ARGS__) : (__VA_ARGS__); })
#define maybe(func, ...) maybeRun(callstack_autoClearCaches, func, __VA_ARGS__)

struct regionInfo regions_getLoadedRegions(void) {
    if (!dlMapper_init()) {
        return (struct regionInfo) { NULL, 0 };
    }

    vector_region_t toReturn = vector_initializer;
    vector_forEach(struct loadedLibInfo, dlMapper_getLoadedLibraries(), outerElement, {
        vector_iterate(struct pair_ptr, &outerElement->associated->regions, {
            vector_region_push_back(&toReturn, ((struct region) {
                element->first, element->second,
                maybe(strdup, outerElement->absoluteFileName),
                maybe(strdup, outerElement->relativeFileName),
            }));
        })
    })

    if (callstack_autoClearCaches) {
        callstack_clearCaches();
    }
    return (struct regionInfo) { toReturn.content, toReturn.count };
}

static inline void region_destroy(struct region self) {
    maybe((void(*)(const char*)) free, self.name);
    maybe((void(*)(const char*)) free, self.nameRelative);
}

void regions_destroyInfo(struct regionInfo* info) {
    vector_region_t tmp = (vector_region_t) { info->amount, info->amount, info->regions };
    vector_region_destroyWith(&tmp, &region_destroy);
}
