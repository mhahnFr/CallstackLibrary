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

#ifndef demangler_hpp
#define demangler_hpp

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Demangles the given name.
 *
 * If the given name is demangled, the given string is not freed.
 * If the name has not been demangled, the given string is returned.
 *
 * @param name The name to be demangled.
 * @return The demangled name or the given name, if it could not be demangled.
 */
char * callstack_demangle(char * name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* demangler_hpp */
