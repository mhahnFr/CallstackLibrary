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

#ifndef CALLSTACKLIBRARY_CALLSTACKFRAMEINTERNAL_H
#define CALLSTACKLIBRARY_CALLSTACKFRAMEINTERNAL_H

#include <callstack_frame.h>

/**
 * Deducts the binary file information for the given address and stores the
 * information in the given callstack frame.
 *
 * @param self the callstack frame object to fill
 * @param address the address whose binary file information to deduct
 * @param useCache whether fill in cached values
 */
void callstackFrame_translateBinary(struct callstack_frame* self, const void* address, bool useCache, bool includeRegions);

#endif //CALLSTACKLIBRARY_CALLSTACKFRAMEINTERNAL_H