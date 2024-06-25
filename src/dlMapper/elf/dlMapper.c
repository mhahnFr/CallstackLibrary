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

#include <limits.h>
#include <string.h>
#include <unistd.h>

#define _GNU_SOURCE
#define __USE_GNU
#include <link.h>
#undef __USE_GNU
#undef _GNU_SOURCE

#include "../dlMapper_platform.h"

#include "../macho/pair_address.h"

#include "../../parser/file/binaryFile.h"

struct dlMapper_platform_data {
    const void* start;
    vector_loadedLibInfo_t* libs;
};

static inline char* dlMapper_platform_loadExecutableName(void) {
    char* buffer = malloc(PATH_MAX);
    if (buffer == NULL) {
        return NULL;
    }
    ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX - 1);
    if (count == -1) {
        free(buffer);
        return NULL;
    }
    buffer[count] = '\0';
    return buffer;
}

static inline int dlMapper_platform_iterateCallback(struct dl_phdr_info* info, size_t size, void* d) {
    struct dlMapper_platform_data* data = d;

    const char* fileName = info->dlpi_name;
    bool empty = *fileName == '\0';
    if (empty) {
        char* newFileName = dlMapper_platform_loadExecutableName();
        if (newFileName != NULL) {
            fileName = newFileName;
        } else {
            empty = false;
        }
    }
  //  pair_address_t addresses = dlMapper_platform_loadELF(...);
    vector_loadedLibInfo_push_back(data->libs, (struct loadedLibInfo) {
        NULL,//begin,
        NULL,//end,
        empty ? (char*) fileName : strdup(fileName),
        binaryFile_toAbsolutePath(fileName),
        binaryFile_toRelativePath(fileName),
        NULL == data->start
    });
    __builtin_printf("%s\n", fileName);
    return 0;
}

static inline void* dlMapper_platform_loadLCSAddress(void) {
    // TODO: Implement
    return NULL;
}

bool dlMapper_platform_loadLoadedLibraries(vector_loadedLibInfo_t* libs) {
    struct dlMapper_platform_data data = (struct dlMapper_platform_data) {
        dlMapper_platform_loadLCSAddress(),
        libs
    };

    return dl_iterate_phdr(dlMapper_platform_iterateCallback, &data) == 0;
}
