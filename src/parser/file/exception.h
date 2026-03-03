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

#ifdef DEBUG
# include <stdio.h>
#endif

struct binaryFileException {
    enum type {
        binaryFileExceptionType_unknown,

        binaryFileExceptionType_unsupported,
        binaryFileExceptionType_failed,
        binaryFileExceptionType_failedAllocation,
        binaryFileExceptionType_invalid,
        binaryFileExceptionType_empty,
    } code;
    const char* fileName,
              * message;
};

#define BFE_THROW_RAW(theCode, theFileName, theMessage) \
    THROW1(struct binaryFileException, {                \
        .code = binaryFileExceptionType_##theCode,      \
        .fileName = (theFileName),                      \
        .message = (theMessage),                        \
    })

#define BFE_THROW(theCode, concreteFilePtr, theMessage) \
    BFE_THROW_RAW(theCode, (concreteFilePtr)->_.fileName.original, theMessage)

#define BFE_THROW_FILE(theCode, concreteFilePtr) \
    BFE_THROW_RAW(theCode, (concreteFilePtr)->_.fileName.original, NULL)

#define BFE_THROW_MSG(theCode, theMessage) \
    BFE_THROW_RAW(theCode, NULL, theMessage)

#define BFE_ALLOC_RAW(size, throwExpr) ({ \
    void* _toReturn = malloc(size);       \
    if (_toReturn == NULL) {              \
        throwExpr;                        \
    }                                     \
    _toReturn;                            \
})

#define BFE_ALLOC_F(size, file, ...) \
    BFE_ALLOC_RAW(size, BFE_THROW(failedAllocation, file __VA_OPT__(,) __VA_ARGS__))

#define BFE_ALLOC_MSG(size, theMessage) \
    BFE_ALLOC_RAW(size, BFE_THROW_MSG(failedAllocation, theMessage))

#ifdef DEBUG
# define BFE_EXCEPTION_HANDLER(exception) do {                                                            \
    const struct binaryFileException* theException = (exception);                                         \
    const char* codeString;                                                                               \
    switch (theException->code) {                                                                         \
        case binaryFileExceptionType_unsupported:      codeString = "unsupported";       break;           \
        case binaryFileExceptionType_failed:           codeString = "failed";            break;           \
        case binaryFileExceptionType_failedAllocation: codeString = "allocation failed"; break;           \
        case binaryFileExceptionType_invalid:          codeString = "invalid";           break;           \
        case binaryFileExceptionType_empty:            codeString = "empty";             break;           \
                                                                                                          \
        default: codeString = "unknown"; break;                                                           \
    }                                                                                                     \
    const bool hasFileName = theException->fileName != NULL,                                              \
                hasMessage = theException->message != NULL;                                               \
    printf("LCS: Binary file exception: %s, %s%s%s%s%s!\n", codeString, hasFileName ? "file name: " : "", \
        hasFileName ? theException->fileName : "", hasFileName && hasMessage ? ", " : "",                 \
        hasMessage ? "message: " : "", hasMessage ? theException->message : "");                          \
} while (0)

#else
# define BFE_EXCEPTION_HANDLER(exception) (void) (exception)

#endif

#endif //CALLSTACKLIBRARY_EXCEPTION_H