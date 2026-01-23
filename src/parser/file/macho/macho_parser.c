/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2024 - 2026  mhahnFr
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

#include "macho_parser.h"

#include <string.h>
#include <mach-o/nlist.h>
#include <mach-o/stab.h>
#include <macho/macho_utils.h>

#include "cache.h"

/*
 Format of the MachO debug symbols:
 
  SO: <path>
  SO: <source_file_name>
 OSO: <full_object_path> <last_modified_time>
 ... <Symbols> ...
  SO: \0
 
 BNSYM: <function address>
   FUN: <linked name> <address>
   FUN: \0 <function's length>
 ENSYM: <function address>
 */

#define machoParser_swap(self, bits, value) macho_maybeSwap(bits, (self)->bytesSwapped, value)

struct machoParser machoParser_create(
    struct symtab_command* command, const void* baseAddress,
    const uintptr_t parsingOffset, const bool bytesSwapped, const bool bit64,
    const machoParser_addFunction functionCallback, void* object)
{
    return (struct machoParser) {
        command, baseAddress, bytesSwapped, bit64, parsingOffset,
        functionCallback, object,
        baseAddress + (macho_maybeSwap(32, bytesSwapped, command->stroff) + parsingOffset),
        bit64 ? sizeof(struct nlist_64) : sizeof(struct nlist),
        {
            { .has_value = false },
            NULL, NULL, NULL
        }
    };
}

#define machoParser_handleSymbolBeginImpl(bits, suffix)                                                   \
static inline bool machoParser_handleSymbolBegin##bits(struct machoParser* self,                          \
                                                       const struct nlist##suffix* entry) {               \
    if (self->parsingState.currentFunction.has_value) {                                                   \
        return false;                                                                                     \
    }                                                                                                     \
    self->parsingState.currentFunction = (optional_function_t) { true, function_initializer };            \
    self->parsingState.currentFunction.value.startAddress = machoParser_swap(self, bits, entry->n_value); \
    return true;                                                                                          \
}

machoParser_handleSymbolBeginImpl(32,)
machoParser_handleSymbolBeginImpl(64, _64)

#define machoParser_handleSymbolEndImpl(bits, suffix)                            \
static inline bool machoParser_handleSymbolEnd##bits(struct machoParser* self) { \
    if (!self->parsingState.currentFunction.has_value) {                         \
        return false;                                                            \
    }                                                                            \
    self->functionCallback(self->object, (pair_funcFile_t) {                     \
        self->parsingState.currentFunction.value,                                \
        self->parsingState.currentObjectFile                                     \
    });                                                                          \
    self->parsingState.currentFunction.has_value = false;                        \
    return true;                                                                 \
}

machoParser_handleSymbolEndImpl(32,)
machoParser_handleSymbolEndImpl(64, _64)

#define machoParser_handleSourceInfoImpl(bits, suffix)                                      \
static inline bool machoParser_handleSourceInfo##bits(struct machoParser* self,             \
                                                      const struct nlist##suffix* entry) {  \
    const char* value = self->stringTable + machoParser_swap(self, 32, entry->n_un.n_strx); \
    if (*value == '\0') {                                                                   \
        if (self->parsingState.currentObjectFile == NULL) {                                 \
            /* Beginning of the symbol table source information, ignore. */                 \
            return true;                                                                    \
        }                                                                                   \
        self->parsingState.currentObjectFile = NULL;                                        \
        self->parsingState.path = self->parsingState.sourceFilename = NULL;                 \
    } else if (self->parsingState.path == NULL) {                                           \
        self->parsingState.path = value;                                                    \
    } else {                                                                                \
        self->parsingState.sourceFilename = value;                                          \
    }                                                                                       \
    return true;                                                                            \
}

machoParser_handleSourceInfoImpl(32,)
machoParser_handleSourceInfoImpl(64, _64)

#define machoParser_handleObjectFileImpl(bits, suffix)                                                       \
static inline bool machoParser_handleObjectFile##bits(struct machoParser* self,                              \
                                                      const struct nlist##suffix* entry) {                   \
    if (self->parsingState.currentObjectFile != NULL) {                                                      \
        return false;                                                                                        \
    }                                                                                                        \
    const char* fileName = self->stringTable + machoParser_swap(self, 32, entry->n_un.n_strx);               \
    const uint64_t modified = machoParser_swap(self, bits, entry->n_value);                                  \
    if ((self->parsingState.currentObjectFile = macho_cache_findOrAdd(fileName, modified)) == NULL) {        \
        return false;                                                                                        \
    }                                                                                                        \
    if (self->parsingState.currentObjectFile->directory == NULL) {                                           \
        self->parsingState.currentObjectFile->directory = self->parsingState.path == NULL                    \
                                                        ? NULL : strdup(self->parsingState.path);            \
    }                                                                                                        \
    if (self->parsingState.currentObjectFile->sourceFile == NULL) {                                          \
        self->parsingState.currentObjectFile->sourceFile = self->parsingState.sourceFilename == NULL         \
                                                         ? NULL : strdup(self->parsingState.sourceFilename); \
    }                                                                                                        \
    return true;                                                                                             \
}

machoParser_handleObjectFileImpl(32,)
machoParser_handleObjectFileImpl(64, _64)

#define machoParser_handleFunctionImpl(bits, suffix)                                                          \
static inline bool machoParser_handleFunction##bits(struct machoParser* self,                                 \
                                                    const struct nlist##suffix* entry) {                      \
    if (!self->parsingState.currentFunction.has_value) {                                                      \
        return false;                                                                                         \
    }                                                                                                         \
    const char* value = self->stringTable + machoParser_swap(self, 32, entry->n_un.n_strx);                   \
    if (*value == '\0') {                                                                                     \
        self->parsingState.currentFunction.value.length = machoParser_swap(self, bits, entry->n_value);       \
    } else {                                                                                                  \
        self->parsingState.currentFunction.value.linkedName   = strdup(value);                                \
        self->parsingState.currentFunction.value.startAddress = machoParser_swap(self, bits, entry->n_value); \
    }                                                                                                         \
    return true;                                                                                              \
}

machoParser_handleFunctionImpl(32,)
machoParser_handleFunctionImpl(64, _64)

#define machoParser_handleGeneralEntryImpl(bits, suffix)                                     \
static inline bool machoParser_handleGeneralEntry##bits(struct machoParser* self,            \
                                                        const struct nlist##suffix* entry) { \
    if ((entry->n_type & N_TYPE) != N_SECT) return true;                                     \
                                                                                             \
    self->functionCallback(self->object, (pair_funcFile_t) {                                 \
        (struct function) {                                                                  \
            machoParser_swap(self, bits, entry->n_value),                                    \
            0,                                                                               \
            strdup(self->stringTable + machoParser_swap(self, 32, entry->n_un.n_strx)),      \
            { .has_value = false }                                                           \
        },                                                                                   \
        NULL                                                                                 \
    });                                                                                      \
    return true;                                                                             \
}

machoParser_handleGeneralEntryImpl(32,)
machoParser_handleGeneralEntryImpl(64, _64)

#define machoParser_handleEntryImpl(bits, suffix)                                     \
static inline bool machoParser_handleEntry##bits(struct machoParser* self,            \
                                                 const struct nlist##suffix* entry) { \
    switch (entry->n_type) {                                                          \
        case N_BNSYM: return machoParser_handleSymbolBegin##bits(self, entry);        \
        case N_ENSYM: return machoParser_handleSymbolEnd##bits(self);                 \
                                                                                      \
        case N_SO:  return machoParser_handleSourceInfo##bits(self, entry);           \
        case N_OSO: return machoParser_handleObjectFile##bits(self, entry);           \
                                                                                      \
        case N_FUN: return machoParser_handleFunction##bits(self, entry);             \
                                                                                      \
        default: return machoParser_handleGeneralEntry##bits(self, entry);            \
    }                                                                                 \
}

machoParser_handleEntryImpl(32,)
machoParser_handleEntryImpl(64, _64)

static inline bool machoParser_handleEntry(struct machoParser* self, const void* entryAddress) {
    if (self->bit64) {
        return machoParser_handleEntry64(self, entryAddress);
    }
    return machoParser_handleEntry32(self, entryAddress);
}

bool machoParser_parseSymbolTable(struct machoParser* self) {
    const uint32_t symCnt  = machoParser_swap(self, 32, self->command->nsyms),
                   symOff = machoParser_swap(self, 32, self->command->symoff);
    for (uint32_t i = 0; i < symCnt; ++i) {
        if (!machoParser_handleEntry(self, self->baseAddress + symOff + self->parsingOffset
                                           + i * self->entrySize)) {
            return false;
        }
    }
    return true;
}

void machoParser_destroy(const struct machoParser* self) {
    if (self->parsingState.currentFunction.has_value) {
        function_destroy(&self->parsingState.currentFunction.value);
    }
}