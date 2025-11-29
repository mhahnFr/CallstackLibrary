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

#include <elf.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#define _GNU_SOURCE
# define __USE_GNU
#  include <link.h>
# undef __USE_GNU
#undef _GNU_SOURCE

#include <elf/elfUtils.h>
#include <file/pathUtils.h>

#include "../dlMapper_platform.h"
#include "../pair_address.h"

/**
 * This structure is used to pass the address of our runtime image and
 * the loaded library information vector to the iteration callback.
 */
struct dlMapper_platform_data {
    /** The pointer into our runtime image.    */
    const void* inside;
    /** The loaded library information vector. */
    vector_loadedLibInfo_t* libs;
};

/**
 * Loads the file name of the main executable.
 *
 * @return the allocated file name of the main executable
 */
static inline char* dlMapper_platform_loadExecutableName(void) {
    char* buffer = malloc(PATH_MAX);
    if (buffer == NULL) {
        return NULL;
    }
    const ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX - 1);
    if (count == -1) {
        free(buffer);
        return NULL;
    }
    buffer[count] = '\0';
    return buffer;
}

/**
 * Generates an implementation for loading the ELF program header number.
 *
 * @param bits the amount of bits the implementation shall handle
 */
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

/**
 * Generates an implementation for loading a given ELF file.
 *
 * @param bits the amount of bits the implementation shall handle
 */
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

/**
 * Parses the given ELF file.
 *
 * @param baseAddress the start address of the ELF file
 * @return the start and the end address of the given ELF file
 */
static inline pair_address_t dlMapper_platform_loadELF(const void* baseAddress) {
    const Elf32_Ehdr* header = baseAddress;
    switch (header->e_ident[EI_CLASS]) {
        case ELFCLASS32: return dlMapper_platform_loadELF32(baseAddress, header->e_ident[EI_DATA] == ELFDATA2LSB);
        case ELFCLASS64: return dlMapper_platform_loadELF64(baseAddress, header->e_ident[EI_DATA] == ELFDATA2LSB);

        default: break;
    }

    return (pair_address_t) { NULL, NULL };
}

/**
 * Loads the address the given runtime image can be found at.
 *
 * @param info the runtime image info
 * @return the address the runtime image starts at
 */
static inline void* dlMapper_platform_loadELFLoadedAddress(struct dl_phdr_info* info) {
    for (unsigned i = 0; i < info->dlpi_phnum; ++i) {
        if (info->dlpi_phdr[i].p_type == PT_LOAD) {
            return ((void*) info->dlpi_addr) + info->dlpi_phdr[i].p_vaddr;
        }
    }
    return NULL;
}

/**
 * Creates a loaded library information from the given info and stores it in
 * the given payload.
 *
 * @param info the system info
 * @param size the size of the passed info structure
 * @param d the payload
 * @return whether to continue iterating
 */
static inline int dlMapper_platform_iterateCallback(struct dl_phdr_info* info, const size_t size, void* d) {
    (void) size;

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
    void* loadedAddress = dlMapper_platform_loadELFLoadedAddress(info);
    if (loadedAddress == NULL) {
        return 0;
    }
    pair_address_t addresses = dlMapper_platform_loadELF(loadedAddress);
    vector_push_back(data->libs, ((struct loadedLibInfo) {
        addresses.first,
        addresses.second,
        info->dlpi_addr,
        empty ? (char*) fileName : strdup(fileName),
        path_toAbsolutePath(fileName),
        path_toRelativePath(fileName),
        data->inside >= addresses.first && data->inside <= addresses.second,
        NULL
    }));
    return 0;
}

/**
 * Loads an address pointing into our runtime image.
 *
 * @return a pointer into our runtime image
 */
static inline void* dlMapper_platform_loadLCSAddress(void) {
    volatile void* me = NULL;
    me = &dlMapper_platform_loadLCSAddress;
    return (void*) me;
}

bool dlMapper_platform_loadLoadedLibraries(vector_loadedLibInfo_t* libs) {
    struct dlMapper_platform_data data = (struct dlMapper_platform_data) {
        dlMapper_platform_loadLCSAddress(),
        libs
    };

    return dl_iterate_phdr(dlMapper_platform_iterateCallback, &data) == 0;
}
