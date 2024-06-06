/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024  mhahnFr
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

#include <string.h>
#include <mach-o/dyld.h>

#include "pair_address.h"

#include "../dlMapper_platform.h"

#include "../../parser/file/macho/macho_utils.h"

static inline const void* dlMapper_platform_loadMachO64(const struct mach_header_64* header, const bool bytesSwapped) {
    uint64_t vmsize = 0;
    struct load_command* lc = (void*) header + sizeof(struct mach_header_64);
    for (uint32_t i = 0; i < macho_maybeSwap(32, bytesSwapped, header->ncmds); ++i) {
        switch (macho_maybeSwap(32, bytesSwapped, lc->cmd)) {
            case LC_SEGMENT_64: {
                struct segment_command_64* cmd = (void*) lc;
                if (strcmp(cmd->segname, SEG_TEXT) == 0) {
                    vmsize = macho_maybeSwap(64, bytesSwapped, cmd->vmsize);
                }
                break;
            }
        }

        lc = (void*) lc + macho_maybeSwap(32, bytesSwapped, lc->cmdsize);
    }
    return (void*) header + vmsize;
}

static inline const void* dlMapper_platform_loadMachO32(const struct mach_header* header, const bool bytesSwapped) {
    uint32_t vmsize = 0;
    struct load_command* lc = (void*) header + sizeof(struct mach_header);
    for (uint32_t i = 0; i < macho_maybeSwap(32, bytesSwapped, header->ncmds); ++i) {
        switch (macho_maybeSwap(32, bytesSwapped, lc->cmd)) {
            case LC_SEGMENT: {
                struct segment_command* cmd = (void*) lc;
                if (strcmp(cmd->segname, SEG_TEXT) == 0) {
                    vmsize = macho_maybeSwap(32, bytesSwapped, cmd->vmsize);
                }
                break;
            }
        }

        lc = (void*) lc + macho_maybeSwap(32, bytesSwapped, lc->cmdsize);
    }
    return (void*) header + vmsize;
}

static inline pair_address_t dlMapper_platform_loadMachO(const struct mach_header* header) {
    const void* end = NULL;
    switch (header->magic) {
        case MH_MAGIC_64:
        case MH_CIGAM_64: end = dlMapper_platform_loadMachO64((void*) header, header->magic == MH_CIGAM_64); break;

        case MH_MAGIC:
        case MH_CIGAM: end = dlMapper_platform_loadMachO32(header, header->magic == MH_CIGAM); break;

        default: abort(); // TODO: Make sure no fat handling is needed
    }

    return (struct pair_address) { header, end };
}

bool dlMapper_platform_loadLoadedLibraries(vector_loadedLibInfo_t* libs) {
    const uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; ++i) {
        const pair_address_t addresses = dlMapper_platform_loadMachO(_dyld_get_image_header(i));
        vector_loadedLibInfo_push_back(libs, (struct loadedLibInfo) {
            addresses.first,
            addresses.second,
            strdup(_dyld_get_image_name(i))
        });
    }
    return true;
}
