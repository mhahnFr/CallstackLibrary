/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
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

#ifndef __lcs_callstack_create_h
#define __lcs_callstack_create_h

#include "callstack.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the given callstack object.
 *
 * @param self The callstack object.
 */
static inline void callstack_create(struct callstack* self) {
    self->backtraceSize     = 0;
    self->frameCount        = 0;
    self->frames            = NULL;
    self->translationStatus = NONE;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __lcs_callstack_create_h */
