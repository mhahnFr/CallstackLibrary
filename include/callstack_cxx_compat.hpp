/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024, 2026  mhahnFr
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

#ifndef _lcs_callstack_cxx_compat_hpp
#define _lcs_callstack_cxx_compat_hpp

#if __cplusplus >= 201103
# define LCS_NOEXCEPT  noexcept
# define LCS_CONSTEXPR constexpr
# define LCS_OVERRIDE  override
# define LCS_NULL      nullptr

# define LCS_CXX11

#else
# define LCS_NOEXCEPT  throw()
# define LCS_CONSTEXPR
# define LCS_OVERRIDE
# define LCS_NULL      NULL
#endif

#endif /* _lcs_callstack_cxx_compat_hpp */
