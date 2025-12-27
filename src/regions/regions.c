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

typedef_vector_named(region, struct region);

/**
 * Executes the given function, passing the given arguments if the given
 * condition evaluates to @c true .
 *
 * @param b the condition
 * @param func the function to possibly execute
 * @param ... the arguments to pass to the given function
 * @return the return value of the given function or the given arguments
 */
#define maybeRun(b, func, ...) ({ (b) ? (func)(__VA_ARGS__) : (__VA_ARGS__); })

/**
 * Executes the given function if the given condition evaluates to @c true .
 *
 * @param b the condition
 * @param func the function to possibly execute
 * @return the return value of the given function or @c NULL
 */
#define maybeRunV(b, func) ((b) ? (func)() : (void) NULL)

/**
 * Executes the given function if @c callstack_autoClearCaches is @c true .
 *
 * @param func the function to possibly execute
 * @param ... the arguments to pass to the given function
 * @return the return value of the function or the given arguments
 */
#define maybe(func, ...) maybeRun(callstack_autoClearCaches, func, __VA_ARGS__)

/**
 * Executes the given function if @c callstack_autoClearCaches is @c true .
 *
 * @param func the function to possibly execute
 * @return the return value of the given function or @c NULL
 */
#define maybeV(func) maybeRunV(callstack_autoClearCaches, func)

struct regionInfo regions_getLoadedRegions(void) {
    if (!dlMapper_init()) {
        return (struct regionInfo) { NULL, 0 };
    }

    vector_region_t toReturn = vector_initializer;
    vector_forEach(dlMapper_getLoadedLibraries(), outerElement, {
        if (!loadedLibInfo_prepare(outerElement)) {
            continue;
        }
        vector_iterate(binaryFile_getRegions(outerElement->associated), {
            vector_push_back(&toReturn, ((struct region) {
                element->first, element->second,
                maybe(strdup, outerElement->absoluteFileName),
                maybe(strdup, outerElement->relativeFileName),
            }));
        });
    });

    if (callstack_autoClearCaches) {
        callstack_clearCaches();
    }
    return (struct regionInfo) { toReturn.content, toReturn.count };
}

struct regionInfo regions_getTLSRegions(void) {
    if (!dlMapper_init()) {
        return (struct regionInfo) { NULL, 0 };
    }

    vector_region_t toReturn = vector_initializer;
    vector_forEach(dlMapper_getLoadedLibraries(), outerElement, {
        if (!loadedLibInfo_prepare(outerElement)) {
            continue;
        }
        vector_pair_ptr_t result = binaryFile_getTLSRegions(outerElement->associated);
        vector_iterate(&result, {
            vector_push_back(&toReturn, ((struct region) {
                element->first, element->second,
                maybe(strdup, outerElement->absoluteFileName),
                maybe(strdup, outerElement->relativeFileName),
            }));
        });
        vector_destroy(&result);
    });

    maybeV(callstack_clearCaches);
    return (struct regionInfo) { toReturn.content, toReturn.count };
}

/**
 * Destroys the given region structure.
 *
 * @param self the region object to be destructed
 */
static inline void regions_destroy(const struct region self) {
    maybe(free, (void*) self.name);
    maybe(free, (void*) self.nameRelative);
}

void regions_destroyInfo(const struct regionInfo* info) {
    vector_region_t tmp = (vector_region_t) { info->amount, info->amount, info->regions };
    vector_destroyWith(&tmp, regions_destroy);
}
