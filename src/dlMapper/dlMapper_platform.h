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

#ifndef dlMapper_platform_h
#define dlMapper_platform_h

#include "dlMapper.h"

/**
 * Loads the loaded runtime images into the given loaded library information vector.
 *
 * @param libs the loaded library information vector
 * @return whether the information could be loaded successfully
 */
bool dlMapper_platform_loadLoadedLibraries(vector_loadedLibInfo_t* libs);

/**
 * @brief Relativizes the given address to the given runtime image info.
 *
 * The return value is the offset into the represented runtime image.
 *
 * @param info the runtime image info to which the given address should be found in
 * @param address the address to be relativized
 * @return the offset into the given runtime image
 */
uintptr_t dlMapper_platform_relativize(const struct loadedLibInfo* info, const void* address);

/**
 * @brief Absolutizes the given address offset.
 *
 * The address offset is interpreted as relative to the given runtime image.
 *
 * @param info the runtime image info used as reference point
 * @param address the address offset into the given runtime image
 * @return the global address within the represented runtime image
 */
uintptr_t dlMapper_platform_absolutize(const struct loadedLibInfo* info, uintptr_t address);

#endif /* dlMapper_platform_h */
