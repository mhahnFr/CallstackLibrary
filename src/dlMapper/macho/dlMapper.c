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

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <mach-o/dyld.h>
#include <mach-o/ldsyms.h>

#include "pair_address.h"

#include "../dlMapper_platform.h"

#include "../../parser/file/binaryFile.h"
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

static inline void* dlMapper_platform_getFileAddress(void* handle, const char* fileName, const char* symbolName) {
    void* symbol = dlsym(handle, symbolName);
    if (symbol == NULL) {
        return NULL;
    }
    Dl_info info;
    int success = dladdr(symbol, &info);
    if (success == 0) {
        return NULL;
    }
    if (strcmp(info.dli_fname, fileName) != 0) {
        return NULL;
    }
    return info.dli_fbase;
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
        const pair_address_t addresses = dlMapper_platform_loadMachO(_dyld_get_image_header(i));
        const char* const fileName = _dyld_get_image_name(i);
        vector_loadedLibInfo_push_back(libs, (struct loadedLibInfo) {
            addresses.first,
            addresses.second,
            strdup(fileName),
            binaryFile_toAbsolutePath(fileName),
            binaryFile_toRelativePath(fileName),
            addresses.first == ourStart
        });
    }

    /*
     * The following handling loads the dyld of macOS. It is not officially
     * loaded, though it is persistently in memory (the dyld loaded us and
     * will unload us later on) and cannot be unloaded.
     *
     * We try to look up some commonly exported functions. If this succeeds,
     * the dyld will tell us its base address.
     *                                                          - mhahnFr
     */
    const char* dyldName = "/usr/lib/dyld";
    // FIXME: Could not load /usr/lib/dyld: tried (...) /usr/lib/dyld: unloadable mach-o file type 7
    void* handle = dlopen(dyldName, RTLD_LAZY | RTLD_LOCAL | RTLD_FIRST);

    const void* dyldStart = NULL;
    const char* guesses[] = {
        "lldb_image_notifier",
        "_dyld_start",
        "_dyld_debugger_notification",
        "gdb_image_notifier",
        NULL
    };
    for (const char** it = guesses; *it != NULL; ++it) {
        dyldStart = dlMapper_platform_getFileAddress(handle == NULL ? RTLD_DEFAULT : handle, dyldName, *it);
        if (dyldStart != NULL) break;
    }

    if (dyldStart == NULL) {
        // TODO: Load from file, try it with symbols from the dysymtab
        printf("LCS: Warning: Could not find /usr/lib/dyld in memory.");
    } else {
        const pair_address_t addresses = dlMapper_platform_loadMachO(dyldStart);
        vector_loadedLibInfo_push_back(libs, (struct loadedLibInfo) {
            addresses.first,
            addresses.second,
            strdup(dyldName),
            binaryFile_toAbsolutePath(dyldName),
            binaryFile_toRelativePath(dyldName),
            false
        });
    }
    if (handle != NULL) {
        dlclose(handle);
    }

    return true;
}
