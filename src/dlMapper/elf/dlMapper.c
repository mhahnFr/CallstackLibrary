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

#include <elf.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#define _GNU_SOURCE
# define __USE_GNU
#  include <link.h>
# undef __USE_GNU
#undef _GNU_SOURCE

#ifdef LCS_BUILD_DYLIB
# include <callstack.h>
#endif

#include <elf/elfUtils.h>
#include <file/pathUtils.h>

#include "../dlMapper_platform.h"
#include "../pair_address.h"

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

#define dlMapper_platform_loadEPHNum(bits)                                                                     \
static inline uint32_t dlMapper_platform_loadEPHNum##bits(const Elf##bits##_Ehdr* header, bool littleEndian) { \
    const uint16_t e_phnum = ELF_TO_HOST(16, header->e_phnum, littleEndian);                                   \
    if (e_phnum != PN_XNUM) {                                                                                  \
        return e_phnum;                                                                                        \
    }                                                                                                          \
                                                                                                               \
    Elf##bits##_Shdr* sect = ((void*) header) + ELF_TO_HOST(bits, header->e_shoff, littleEndian);              \
    return ELF_TO_HOST(32, sect->sh_info, littleEndian);                                                       \
}

dlMapper_platform_loadEPHNum(32)
dlMapper_platform_loadEPHNum(64)

#define dlMapper_platform_loadELF_impl(bits)                                                        \
static inline pair_address_t dlMapper_platform_loadELF##bits(const void* base, bool littleEndian) { \
    const Elf##bits##_Ehdr* header = base;                                                          \
                                                                                                    \
    const void* biggest = NULL;                                                                     \
    const uint32_t e_phnum = dlMapper_platform_loadEPHNum##bits(header, littleEndian);              \
    for (uint32_t i = 0; i < e_phnum; ++i) {                                                        \
        Elf##bits##_Phdr* seg = ((void*) header) + ELF_TO_HOST(bits, header->e_phoff, littleEndian) \
                                + i * ELF_TO_HOST(16, header->e_phentsize, littleEndian);           \
        const void* address = base + ELF_TO_HOST(bits, seg->p_offset, littleEndian)                 \
                             + ELF_TO_HOST(bits, seg->p_memsz, littleEndian);                       \
        if (biggest == NULL || biggest < address) {                                                 \
            biggest = address;                                                                      \
        }                                                                                           \
    }                                                                                               \
    return (pair_address_t) { base, biggest };                                                      \
}

dlMapper_platform_loadELF_impl(32)
dlMapper_platform_loadELF_impl(64)

static inline pair_address_t dlMapper_platform_loadELF(const void* baseAddress) {
    const Elf32_Ehdr* header = baseAddress;
    switch (header->e_ident[EI_CLASS]) {
        case ELFCLASS32: return dlMapper_platform_loadELF32(baseAddress, header->e_ident[EI_DATA] == ELFDATA2LSB);
        case ELFCLASS64: return dlMapper_platform_loadELF64(baseAddress, header->e_ident[EI_DATA] == ELFDATA2LSB);
    }

    return (pair_address_t) { NULL, NULL };
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
    pair_address_t addresses = dlMapper_platform_loadELF((void*) info->dlpi_addr);
    vector_loadedLibInfo_push_back(data->libs, (struct loadedLibInfo) {
        addresses.first,
        addresses.second,
        empty ? (char*) fileName : strdup(fileName),
        path_toAbsolutePath(fileName),
        path_toRelativePath(fileName),
        addresses.first == data->start,
        NULL
    });
    return 0;
}

static inline void* dlMapper_platform_loadLCSAddress(void) {
    void* ourStart = NULL;

#ifdef LCS_BUILD_DYLIB
    Dl_info info;
    if (dladdr(&callstack_new, &info) == 0) {
        return ourStart;
    }
    ourStart = info.dli_fbase;
#endif

    return ourStart;
}

bool dlMapper_platform_loadLoadedLibraries(vector_loadedLibInfo_t* libs) {
    struct dlMapper_platform_data data = (struct dlMapper_platform_data) {
        dlMapper_platform_loadLCSAddress(),
        libs
    };

    return dl_iterate_phdr(dlMapper_platform_iterateCallback, &data) == 0;
}
