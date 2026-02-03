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

#ifndef CALLSTACKLIBRARY_CACHE_H
#define CALLSTACKLIBRARY_CACHE_H

#include <callstack_internals.h>

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

#endif //CALLSTACKLIBRARY_CACHE_H