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

#include <stdio.h>

#include <mach-o/dyld.h>
#include <mach-o/dyld_images.h>

#include <file/pathUtils.h>
#include <macho/fat_handler.h>
#include <macho/macho_utils.h>

#include "../dlMapper_platform.h"
#include "../pair_address.h"

typedef_pair_named(machoInfo, const void*, uintptr_t);
typedef_pair_named(machoDef, pair_address_t, uintptr_t);

/**
 * Generates an implementation for a Mach-O function loader.
 *
 * @param bits the amount of bits the implementation should be generated for
 * @param suffix the optional suffix for the native data structures
 */
#define dlMapper_platform_loadMachOFunc(bits, suffix)                                 \
static inline pair_machoInfo_t dlMapper_platform_loadMachO##bits(                     \
    const struct mach_header##suffix* header, const bool bytesSwapped, vector_pair_ptr_t* vec) {              \
    uint##bits##_t vmsize = 0, vmaddr = 0;                                            \
    struct load_command* lc = (void*) header + sizeof(struct mach_header##suffix);    \
    for (uint32_t i = 0; i < macho_maybeSwap(32, bytesSwapped, header->ncmds); ++i) { \
        switch (macho_maybeSwap(32, bytesSwapped, lc->cmd)) {                         \
            case LC_SEGMENT##suffix: {                                                \
                struct segment_command##suffix* cmd = (void*) lc;                     \
                if (strcmp(cmd->segname, SEG_TEXT) == 0) {                            \
                    vmsize = macho_maybeSwap(bits, bytesSwapped, cmd->vmsize);        \
                    vmaddr = macho_maybeSwap(bits, bytesSwapped, cmd->vmaddr);        \
                }                                                                     \
                if (cmd->initprot & 2 && cmd->initprot & 1) {                                                          \
                    vector_push_back(vec, ((pair_ptr_t) { cmd->vmaddr, cmd->vmaddr + cmd->vmsize })); \
                }                                                                                                              \
                break;                                                                \
            }                                                                         \
        }                                                                             \
                                                                                      \
        lc = (void*) lc + macho_maybeSwap(32, bytesSwapped, lc->cmdsize);             \
    }                                                                                 \
    return make_pair_machoInfo((void*) header + vmsize, vmaddr);                      \
}

dlMapper_platform_loadMachOFunc(32,)
dlMapper_platform_loadMachOFunc(64, _64)

/**
 * Loads the given Mach-O file.
 *
 * @param header the start pointer of the Mach-O file
 * @param fileName the file name of the Mach-O file
 * @return the start and the end address of the Mach-O file
 */
static inline pair_machoDef_t dlMapper_platform_loadMachO(const struct mach_header* header, const char* fileName, vector_pair_ptr_t* vec) {
    pair_machoInfo_t info = { NULL, 0 };
    switch (header->magic) {
        case MH_MAGIC_64:
        case MH_CIGAM_64: info = dlMapper_platform_loadMachO64((void*) header, header->magic == MH_CIGAM_64, vec); break;

        case MH_MAGIC:
        case MH_CIGAM: info = dlMapper_platform_loadMachO32(header, header->magic == MH_CIGAM, vec); break;

        case FAT_MAGIC:
        case FAT_MAGIC_64: return dlMapper_platform_loadMachO(macho_parseFat((void*) header, false, fileName), fileName, vec);

        case FAT_CIGAM:
        case FAT_CIGAM_64: return dlMapper_platform_loadMachO(macho_parseFat((void*) header, true, fileName), fileName, vec);

        default: break;
    }

    return make_pair_machoDef((pair_address_t) { header, info.first }, info.second);
}

/**
 * Parses the given Mach-O file and stores the result in the cache.
 *
 * @param libs the vector to store the loaded library info in
 * @param fileName the file name of the Mach-O file
 * @param header the start pointer of the Mach-O file
 * @param inside the pointer pointing inside our runtime image
 */
static inline void dlMapper_platform_pushLoadedLib(vector_loadedLibInfo_t*   libs,
                                                   const char*               fileName,
                                                   const struct mach_header* header,
                                                   const void*               inside) {
    vector_pair_ptr_t vec = vector_initializer;
    const pair_machoDef_t addresses = dlMapper_platform_loadMachO(header, fileName, &vec);
    // TODO: Support multiple slices for each loaded library
    vector_push_back(libs, ((struct loadedLibInfo) {
        addresses.first.first,
        addresses.first.second,
        addresses.second,
        strdup(fileName),
        path_toAbsolutePath(fileName),
        path_toRelativePath(fileName),
        inside >= addresses.first.first && inside <= addresses.first.second,
        NULL,
        vec
    }));
}

bool dlMapper_platform_loadLoadedLibraries(vector_loadedLibInfo_t* libs) {
    volatile const void* inside = NULL;
    inside = &dlMapper_platform_loadLoadedLibraries;

    const uint32_t count = _dyld_image_count();
    vector_reserve(libs, count + 1);
    for (uint32_t i = 0; i < count; ++i) {
        dlMapper_platform_pushLoadedLib(libs, _dyld_get_image_name(i), _dyld_get_image_header(i), (void*) inside);
    }

    struct task_dyld_info dyldInfo;
    mach_msg_type_number_t infoCount = TASK_DYLD_INFO_COUNT;
    if (task_info(mach_task_self_, TASK_DYLD_INFO, (task_info_t) &dyldInfo, &infoCount) == KERN_SUCCESS) {
        const struct dyld_all_image_infos* infos = (void*) dyldInfo.all_image_info_addr;
        dlMapper_platform_pushLoadedLib(libs, infos->dyldPath, infos->dyldImageLoadAddress, (void*) inside);
    } else {
        printf("LCS: Warning: Failed to load the dynamic loader. Callstacks might be truncated.\n");
    }

    return true;
}

uintptr_t dlMapper_platform_relativize(const struct loadedLibInfo* info, const void* address) {
    return (uintptr_t) (address - info->begin) + info->relocationOffset;
}

uintptr_t dlMapper_platform_absolutize(const struct loadedLibInfo* info, const uintptr_t address) {
    return address + (uintptr_t) info->begin - info->relocationOffset;
}