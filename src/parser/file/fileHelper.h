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

#ifndef fileHelper_h
#define fileHelper_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Converts the given path to a relative path.
 *
 * @param path the path to create a relative one from
 * @return a newly allocated relative path
 */
char * lcs_toRelativePath(char * path);
char * lcs_toCanonicalPath(char * path);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* fileHelper_h */
