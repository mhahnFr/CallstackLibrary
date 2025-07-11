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

#ifndef callstack_demangling_demangler_h
#define callstack_demangling_demangler_h

/**
 * @brief Attempts to demangle the given name.
 *
 * Currently, Swift and C++ demangling is supported.
 *
 * @param name the name to be demangled
 * @return an allocated demangled name or the input if not mangled
 */
char* callstack_demangle(char* name);

#endif /* callstack_demangling_demangler_h */
