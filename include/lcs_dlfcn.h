/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023  mhahnFr
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

#ifndef lcs_dlfcn_h
#define lcs_dlfcn_h

#ifndef _GNU_SOURCE
 #define UNDEF_GNU_SOURCE
 #define _GNU_SOURCE
#endif

#ifndef __USE_GNU
 #define UNDEF_USE_GNU
 #define __USE_GNU
#endif

#include <dlfcn.h>

#ifdef UNDEF_GNU_SOURCE
 #undef _GNU_SOURCE
 #undef UNDEF_GNU_SOURCE
#endif

#ifdef UNDEF_USE_GNU
 #undef __USE_GNU
 #undef UNDEF_USE_GNU
#endif

#endif /* lcs_dlfcn_h */
