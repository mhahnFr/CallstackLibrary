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

#ifndef dlMapper_h
#define dlMapper_h

#include <stdbool.h>

#include "vector_loadedLibInfo.h"

/**
 * @brief Initializes the dlMapper.
 *
 * Does nothing if it has already been initialized; if that is the case,
 * @c true will be returned.
 *
 * @return whether the dlMapper has been successfully initialized
 */
bool dlMapper_init(void);

/**
 * Returns whether the dlMapper has already been initialized.
 *
 * @return whether the dlMapper is initialized
 */
bool dlMapper_isInited(void);

/**
 * Returns the loaded library info the given pointer is associated with.
 *
 * @param address the address whose runtime image to find
 * @return the associated loaded library info object or @c NULL if not in any
 * loaded library
 */
struct loadedLibInfo* dlMapper_libInfoForAddress(const void* address);

/**
 * Returns the runtime image info for the runtime image of the given name.
 *
 * @param fileName the file name of the runtime image
 * @return the associated runtime image info or @c NULL if not found
 */
struct loadedLibInfo* dlMapper_libInfoForFileName(const char* fileName);

/**
 * Returns the loaded runtime image infos.
 *
 * @return the loaded runtime image infos
 */
const vector_loadedLibInfo_t* dlMapper_getLoadedLibraries(void);

/**
 * Deinitializes the dlMapper.
 */
void dlMapper_deinit(void);

#endif /* dlMapper_h */
