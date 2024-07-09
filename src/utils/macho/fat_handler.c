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

#include <Availability.h>

#if defined(__BLOCKS__) && defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && defined(MAC_OS_VERSION_13_0) \
    && __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_13_0
 #define HAS_MACH_O_UTILS
 #include <mach-o/utils.h>
#else
 #include <mach-o/arch.h>
 #include <sys/sysctl.h>
#endif

#include <macho/macho_utils.h>

#include "fat_handler.h"

void* macho_parseFat(const struct fat_header* fatHeader, bool bitsReversed, const char* fileName) {
#ifdef HAS_MACH_O_UTILS
    (void) bitsReversed;

    __block uint64_t fileOffset;

    const int result = macho_best_slice(fileName,
                                        ^ (const struct mach_header* header, uint64_t offset, size_t size) {
        (void) header;
        (void) size;

        fileOffset = offset;
    });
    return result == 0 ? (void*) fatHeader + fileOffset : NULL;
#else
    (void) fileName;

    uint32_t cputype,
             cpusubtype;
    size_t len = sizeof(uint32_t);
    if (sysctlbyname("hw.cputype", &cputype, &len, NULL, 0) != 0
        || sysctlbyname("hw.cpusubtype", &cpusubtype, &len, NULL, 0) != 0) {
        return false;
    }
    uint64_t offset;
    switch (fatHeader->magic) {
        case FAT_MAGIC_64:
        case FAT_CIGAM_64: {
            const struct fat_arch_64* best = NXFindBestFatArch_64(macho_maybeSwap(32, bitsReversed, cputype),
                                                                  macho_maybeSwap(32, bitsReversed, cpusubtype),
                                                                  (void*) fatHeader + sizeof(struct fat_header),
                                                                  macho_maybeSwap(32, bitsReversed, fatHeader->nfat_arch));
            if (best == NULL) {
                return NULL;
            }
            offset = macho_maybeSwap(64, bitsReversed, best->offset);
            break;
        }

        case FAT_MAGIC:
        case FAT_CIGAM: {
            const struct fat_arch* best = NXFindBestFatArch(macho_maybeSwap(32, bitsReversed, cputype),
                                                            macho_maybeSwap(32, bitsReversed, cpusubtype),
                                                            (void*) fatHeader + sizeof(struct fat_header),
                                                            macho_maybeSwap(32, bitsReversed, fatHeader->nfat_arch));
            if (best == NULL) {
                return NULL;
            }
            offset = macho_maybeSwap(32, bitsReversed, best->offset);
            break;
        }

        default: return NULL;
    }
    return (void*) fatHeader + offset;
#endif
}
