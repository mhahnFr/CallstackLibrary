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

#ifndef __lcs_regions_regions_h
#define __lcs_regions_regions_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

struct region {
    uintptr_t begin, end;

    const char* name, *nameRelative;
};

struct regionInfo {
    struct region* regions;
    size_t amount;
};

struct regionInfo regions_getLoadedRegions(void);

// Important: Gets the TLS (and potentially initializes them) for the calling
// thread. Cares for callstack_autoClearCaches.
struct regionInfo regions_getTLSRegions(void);

void regions_destroyInfo(struct regionInfo* info);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __lcs_regions_regions_h */
