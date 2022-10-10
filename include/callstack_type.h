/*
 * Callstack Library - A library creating human readable call stacks.
 *
 * Copyright (C) 2022  mhahnFr
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
    /**
     * Indicates that the callstack has been generated using the dynamic linker informations.
     */
    DYNAMIC_LINKER,
    /**
     * Indicates that the callstack has been generated using the debug symbols.
     */
    DEBUG_SYMBOLS,
    /**
     * Indcates that an error happened during the translation of the backtrace.
     */
    FAILED,
    /**
     * Indicates that the callstack is not translated in a human readable form.
     */
    NONE
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* callstack_type_h */
