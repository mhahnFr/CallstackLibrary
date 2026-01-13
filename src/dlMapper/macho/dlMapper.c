/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2026  mhahnFr
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

#include "../dlMapper_platform.h"

bool dlMapper_platform_loadLoadedLibraries(vector_binaryFile_t* libs) {
    const uint32_t count = _dyld_image_count();
    vector_reserve(libs, count + 1);
    for (uint32_t i = 0; i < count; ++i) {
        vector_push_back(libs, binaryFile_new(_dyld_get_image_name(i), _dyld_get_image_header(i)));
    }

    struct task_dyld_info dyldInfo;
    mach_msg_type_number_t infoCount = TASK_DYLD_INFO_COUNT;
    if (task_info(mach_task_self_, TASK_DYLD_INFO, (task_info_t) &dyldInfo, &infoCount) == KERN_SUCCESS) {
        const struct dyld_all_image_infos* infos = (void*) dyldInfo.all_image_info_addr;
        vector_push_back(libs, binaryFile_new(infos->dyldPath, infos->dyldImageLoadAddress));
    } else {
        printf("LCS: Warning: Failed to load the dynamic loader. Callstacks might be truncated.\n");
    }

    return true;
}

uintptr_t dlMapper_platform_relativize(const struct binaryFile* info, const void* address) {
    return (uintptr_t) (address - info->startAddress) + info->relocationOffset;
}

uintptr_t dlMapper_platform_absolutize(const struct binaryFile* info, const uintptr_t address) {
    return address + (uintptr_t) info->startAddress - info->relocationOffset;
}