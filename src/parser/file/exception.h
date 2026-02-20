/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2026  mhahnFr
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

#ifndef CALLSTACKLIBRARY_EXCEPTION_H
#define CALLSTACKLIBRARY_EXCEPTION_H

#include <stdbool.h>
#include <try_catch.h>

struct binaryFileException {
    enum type {
        binaryFileExceptionType_unknown,

        binaryFileExceptionType_unsupportedType,
        binaryFileExceptionType_failed,
        binaryFileExceptionType_failedAllocation,
        binaryFileExceptionType_empty,
    } code;
    bool hasMessage;
    union {
        struct binaryFile* file;
        const char* message;
    } payload;
};

#define BFE_THROW(theCode, concreteFilePtr)        \
    THROW1(struct binaryFileException, {           \
        .code = binaryFileExceptionType_##theCode, \
        .hasMessage = false,                       \
        .payload.file = &(concreteFilePtr)->_,     \
    })

#define BFE_ALLOC_RAW(size, throwExpr) ({ \
    void* _toReturn = malloc(size);       \
    if (_toReturn == NULL) {              \
        throwExpr;                        \
    }                                     \
    _toReturn;                            \
})

#define BFE_ALLOC_F(size, file) BFE_ALLOC_RAW(size, BFE_THROW(failedAllocation, file))

#define BFE_ALLOC_MSG(size, theMessage)                  \
BFE_ALLOC_RAW(size, THROW1(struct binaryFileException, { \
    .code = binaryFileExceptionType_failedAllocation,    \
    .hasMessage = true,                                  \
    .payload.message = (theMessage),                     \
}))

#endif //CALLSTACKLIBRARY_EXCEPTION_H