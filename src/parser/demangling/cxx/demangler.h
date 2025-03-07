/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2022, 2025  mhahnFr
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

#ifndef callstack_demangling_cxx_demangler_h
#define callstack_demangling_cxx_demangler_h

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
char* callstack_demangle_cxx(char* name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* callstack_demangling_cxx_demangler_h */
