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

#ifndef machoFile_h
#define machoFile_h

#include <stddef.h>

#include "../binaryFile.h"

struct machoFile {
    struct binaryFile _;
    // ...
};

struct machoFile * machoFile_new(void);
void machoFile_create(struct machoFile * self);

static inline struct machoFile * machoFileOrNull(struct binaryFile * self) {
    return self->type == MACHO_FILE ? self->concrete : NULL;
}

char * machoFile_addr2String(struct binaryFile * self, Dl_info * info, void * address);

void machoFile_destroy(struct binaryFile * self);
void machoFile_delete(struct binaryFile * self);

#endif /* machoFile_h */
