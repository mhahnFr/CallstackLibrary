/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2025  mhahnFr
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

#ifndef __utils_file_pathUtils_h
#define __utils_file_pathUtils_h

/**
 * @brief Returns the absolute path of the given path name.
 *
 * If @c CXX_FUNCTIONS is defined, the absolute path is returned. Otherwise,
 * the given path name is simply duplicated. In any case, the return value
 * should be freed after use.
 *
 * @param path the path name
 * @return the allocated, absolute path name
 */
char* path_toAbsolutePath(const char* path);

/**
 * @brief Returns the absolute path of the given path name.
 *
 * If @c CXX_FUNCTIONS is defined, the absolute path is returned. Otherwise,
 * the given path name is simply duplicated. In any case, the return value
 * should be freed after use.
 * <br><br>
 * The given path name is freed after it has been converted.
 *
 * @param path the path name
 * @return the allocated, absolute path name
 */
char* path_toAbsolutePathFree(char* path);

/**
 * @brief Returns the relative path of the given path name.
 *
 * If @c CXX_FUNCTIONS is defined, the relative path is returned. Otherwise,
 * the given path name is simply duplicated. In any case, the return value
 * should be freed after use.
 *
 * @param path the path name
 * @return the allocated, relative path name
 */
char* path_toRelativePath(const char* path);

/**
 * @brief Returns the relative path of the given path name.
 *
 * If @c CXX_FUNCTIONS is defined, the relative path is returned. Otherwise,
 * the given path name is simply duplicated. In any case, the return value
 * should be freed after use.
 * <br><br>
 * The given path name is freed after it has been converted.
 *
 * @param path the path name
 * @return the allocated, relative path name
 */
char* path_toRelativePathFree(char* path);

#endif /* __utils_file_pathUtils_h */
