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

#ifndef elfFile_h
#define elfFile_h

#include "../binaryFile.h"

struct elfFile {
    struct binaryFile _;
    // ...
};

struct elfFile * elfFile_new(void);
void elfFile_create(struct elfFile * self);

static inline struct elfFile * elfFileOrNull(struct binaryFile * self) {
    return self->type == ELF_FILE ? self->concrete : NULL;
}

char * elfFile_addr2String(struct binaryFile * self, Dl_info * info, void * address);

void elfFile_destroy(struct binaryFile * self);
void elfFile_delete(struct binaryFile * self);

#endif /* elfFile_h */
