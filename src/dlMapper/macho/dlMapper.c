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

#include <stdio.h>
#include <string.h>

#include <mach-o/dyld.h>
#include <mach-o/dyld_images.h>
#include <mach-o/ldsyms.h>

#include <file/pathUtils.h>
#include <macho/fat_handler.h>
#include <macho/macho_utils.h>

#include "../pair_address.h"
#include "../dlMapper_platform.h"

/**
 * Parses the given 64 bit Mach-O file.
 *
 * @param header the start pointer of the Mach-O file
 * @param bytesSwapped whether the bytes need to be swapped to match the host byte order
 * @return the end address of the Mach-O file
 */
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

/**
 * Parses the given 32 bit Mach-O file.
 *
 * @param header the start pointer of the Mach-O file
 * @param bytesSwapped whether the bytes need to be swapped to match the host byte order
 * @return the end address of the Mach-O file
 */
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

/**
 * Loads the given Mach-O file.
 *
 * @param header the start pointer of the Mach-O file
 * @param fileName the file name of the Mach-O file
 * @return the start and the end address of the Mach-O file
 */
static inline pair_address_t dlMapper_platform_loadMachO(const struct mach_header* header, const char* fileName) {
    const void* end = NULL;
    switch (header->magic) {
        case MH_MAGIC_64:
        case MH_CIGAM_64: end = dlMapper_platform_loadMachO64((void*) header, header->magic == MH_CIGAM_64); break;

        case MH_MAGIC:
        case MH_CIGAM: end = dlMapper_platform_loadMachO32(header, header->magic == MH_CIGAM); break;

        case FAT_MAGIC:
        case FAT_MAGIC_64: return dlMapper_platform_loadMachO(macho_parseFat((void*) header, false, fileName), fileName);

        case FAT_CIGAM:
        case FAT_CIGAM_64: return dlMapper_platform_loadMachO(macho_parseFat((void*) header, true, fileName), fileName);
    }

    return (struct pair_address) { header, end };
}

/**
 * Parses the given Mach-O file and stores the result in the cache.
 *
 * @param libs the vector to store the loaded library info in
 * @param fileName the file name of the Mach-O file
 * @param header the start pointer of the Mach-O file
 * @param ourStart the start pointer of our runtime image
 */
static inline void dlMapper_platform_pushLoadedLib(vector_loadedLibInfo_t*   libs,
                                                   const char*               fileName,
                                                   const struct mach_header* header,
                                                   const void*               ourStart) {
    const pair_address_t addresses = dlMapper_platform_loadMachO(header, fileName);
    vector_loadedLibInfo_push_back(libs, (struct loadedLibInfo) {
        addresses.first,
        addresses.second,
        strdup(fileName),
        path_toAbsolutePath(fileName),
        path_toRelativePath(fileName),
        addresses.first == ourStart,
        NULL
    });
}

bool dlMapper_platform_loadLoadedLibraries(vector_loadedLibInfo_t* libs) {
    const void* ourStart;

#ifdef LCS_BUILD_DYLIB
    ourStart = &_mh_dylib_header;
#else
    ourStart = NULL;
#endif

    const uint32_t count = _dyld_image_count();
    vector_loadedLibInfo_reserve(libs, count + 1);
    for (uint32_t i = 0; i < count; ++i) {
        dlMapper_platform_pushLoadedLib(libs, _dyld_get_image_name(i), _dyld_get_image_header(i), ourStart);
    }

    struct task_dyld_info dyldInfo;
    mach_msg_type_number_t infoCount = TASK_DYLD_INFO_COUNT;
    if (task_info(mach_task_self_, TASK_DYLD_INFO, (task_info_t) &dyldInfo, &infoCount) == KERN_SUCCESS) {
        struct dyld_all_image_infos* infos = (void*) dyldInfo.all_image_info_addr;
        dlMapper_platform_pushLoadedLib(libs, infos->dyldPath, infos->dyldImageLoadAddress, ourStart);
    } else {
        printf("LCS: Warning: Failed to load the dynamic loader. Callstacks might be truncated.\n");
    }

    return true;
}
