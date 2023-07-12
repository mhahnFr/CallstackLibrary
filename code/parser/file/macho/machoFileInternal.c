/*
 * Callstack Library - A library creating human readable call stacks.
 *
 * Copyright (C) 2023  mhahnFr
 *
 * This file is part of the CallstackLibrary. This library is free software:
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string.h>

#include <mach-o/loader.h>

#include "machoFileInternal.h"

static inline bool machoFile_parseFileImpl(struct machoFile * self,
                                           void *             baseAddress,
                                           bool               bitsReversed) {
    // TODO: Implement 32 bit version
    (void) self;
    (void) baseAddress;
    (void) bitsReversed;
    return false;
}

static inline bool machoFile_parseFileImpl64(struct machoFile * self,
                                             void *             baseAddress,
                                             bool               bitsReversed) {
    // TODO: Implement 64 bit version
    (void) self;
    (void) baseAddress;
    (void) bitsReversed;
    return false;
}

bool machoFile_parseFile(struct machoFile * self, void * baseAddress) {
    if (strcmp(self->_.fileName, "/usr/lib/dyld") == 0) {
        // We cannot parse Darwins dynamic linker at the moment.
        return false;
    }
    
    struct mach_header * header = baseAddress;
    switch (header->magic) {
        case MH_MAGIC: return machoFile_parseFileImpl(self, baseAddress, false);
        case MH_CIGAM: return machoFile_parseFileImpl(self, baseAddress, true);
            
        case MH_MAGIC_64: return machoFile_parseFileImpl64(self, baseAddress, false);
        case MH_CIGAM_64: return machoFile_parseFileImpl64(self, baseAddress, true);
    }
    return false;
}

void machoFile_addObjectFile(struct machoFile *  self,
                             struct objectFile * file) {
    file->next        = self->objectFiles;
    self->objectFiles = file;
}
