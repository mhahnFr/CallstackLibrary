/*
 * Callstack Library - A library creating human readable call stacks.
 *
 * Copyright (C) 2022 - 2023  mhahnFr
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

#ifndef callstack_type_h
#define callstack_type_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of the callstack.
 */
enum callstack_type {
    /** The callstack has been translated.    */
    TRANSLATED,
    /** An error happened during translation. */
    FAILED,
    /** Not translated.                       */
    NONE
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* callstack_type_h */
