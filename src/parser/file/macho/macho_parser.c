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
#include <try_catch.h>
#include <mach-o/nlist.h>
#include <mach-o/stab.h>
#include <macho/macho_utils.h>

#include "cache.h"
#include "../exception.h"

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

/**
 * Swap the endianness of the given value if necessary.
 *
 * @param self the Mach-O symbol table parser
 * @param bits the amount of bits
 * @param value the value
 */
#define machoParser_swap(self, bits, value) macho_maybeSwap(bits, (self)->bytesSwapped, value)
#define throw(code, self, message) BFE_THROW_RAW(code, (self)->fileName, message)

struct machoParser machoParser_create(
    struct symtab_command* command, const void* baseAddress,
    const uintptr_t parsingOffset, const bool bytesSwapped, const bool bit64,
    const machoParser_addSymbol symbolCallback, void* object, const char* fileName)
{
    return (struct machoParser) {
        command, baseAddress, bytesSwapped, bit64, parsingOffset,
        symbolCallback, object, fileName, {
            baseAddress + (macho_maybeSwap(32, bytesSwapped, command->stroff) + parsingOffset),
            bit64 ? sizeof(struct nlist_64) : sizeof(struct nlist),
            {
                { .has_value = false },
                NULL, NULL, NULL
            }
        }
    };
}

/**
 * Generates an implementation for the symbol begin handling.
 *
 * @param bits the amount of bits to generate the implementation for
 * @param suffix the suffix to be used for the system provided structures
 */
#define machoParser_handleSymbolBeginImpl(bits, suffix)                                                         \
static inline void machoParser_handleSymbolBegin##bits(struct machoParser* self,                                \
                                                       const struct nlist##suffix* entry) {                     \
    if (self->private.parsingState.currentSymbol.has_value) {                                                   \
        throw(invalid, self, "Symbol begin with active symbol");                                                \
    }                                                                                                           \
    self->private.parsingState.currentSymbol = (optional_symbol_t) { true, symbol_initializer };                \
    self->private.parsingState.currentSymbol.value.startAddress = machoParser_swap(self, bits, entry->n_value); \
}

machoParser_handleSymbolBeginImpl(32,)
machoParser_handleSymbolBeginImpl(64, _64)

/**
 * Generates an implementation for handling symbol endings.
 *
 * @param bits the amount of bits to generate the implementation for
 * @param suffix the suffix to be used for the system provided structures
 */
#define machoParser_handleSymbolEndImpl(bits, suffix)                            \
static inline void machoParser_handleSymbolEnd##bits(struct machoParser* self) { \
    if (!self->private.parsingState.currentSymbol.has_value) {                   \
        throw(invalid, self, "Symbol end without active symbol");                \
    }                                                                            \
    self->symbolCallback(self->object, (pair_symbolFile_t) {                     \
        self->private.parsingState.currentSymbol.value,                          \
        self->private.parsingState.currentObjectFile                             \
    });                                                                          \
    self->private.parsingState.currentSymbol.has_value = false;                  \
}

machoParser_handleSymbolEndImpl(32,)
machoParser_handleSymbolEndImpl(64, _64)

/**
 * Generates an implementation for handling the source file information.
 *
 * @param bits the amount of bits to generate the implementation for
 * @param suffix the suffix to be used for the system provided structures
 */
#define machoParser_handleSourceInfoImpl(bits, suffix)                                              \
static inline void machoParser_handleSourceInfo##bits(struct machoParser* self,                     \
                                                      const struct nlist##suffix* entry) {          \
    const char* value = self->private.stringTable + machoParser_swap(self, 32, entry->n_un.n_strx); \
    if (*value == '\0') {                                                                           \
        if (self->private.parsingState.currentObjectFile == NULL) {                                 \
            /* Beginning of the symbol table source information, ignore. */                         \
            return;                                                                                 \
        }                                                                                           \
        self->private.parsingState.currentObjectFile = NULL;                                        \
        self->private.parsingState.path = self->private.parsingState.sourceFilename = NULL;         \
    } else if (self->private.parsingState.path == NULL) {                                           \
        self->private.parsingState.path = value;                                                    \
    } else {                                                                                        \
        self->private.parsingState.sourceFilename = value;                                          \
    }                                                                                               \
}

machoParser_handleSourceInfoImpl(32,)
machoParser_handleSourceInfoImpl(64, _64)

/**
 * Generates an implementation for handling object file entries.
 *
 * @param bits the amount of bits to generate the implementation for
 * @param suffix the suffix to be used for the system provided data structures
 */
#define machoParser_handleObjectFileImpl(bits, suffix)                                                               \
static inline void machoParser_handleObjectFile##bits(struct machoParser* self,                                      \
                                                      const struct nlist##suffix* entry) {                           \
    if (self->private.parsingState.currentObjectFile != NULL) {                                                      \
        throw(invalid, self, "Handling object file without active object file");                                     \
    }                                                                                                                \
    const char* fileName = self->private.stringTable + machoParser_swap(self, 32, entry->n_un.n_strx);               \
    const uint64_t modified = machoParser_swap(self, bits, entry->n_value);                                          \
    if ((self->private.parsingState.currentObjectFile = macho_cache_findOrAdd(fileName, modified)) == NULL) {        \
        throw(failed, self, "Failed to cache object file");                                                          \
    }                                                                                                                \
    if (self->private.parsingState.currentObjectFile->directory == NULL) {                                           \
        self->private.parsingState.currentObjectFile->directory = self->private.parsingState.path == NULL            \
                                                        ? NULL : strdup(self->private.parsingState.path);            \
    }                                                                                                                \
    if (self->private.parsingState.currentObjectFile->sourceFile == NULL) {                                          \
        self->private.parsingState.currentObjectFile->sourceFile = self->private.parsingState.sourceFilename == NULL \
                                                         ? NULL : strdup(self->private.parsingState.sourceFilename); \
    }                                                                                                                \
}

machoParser_handleObjectFileImpl(32,)
machoParser_handleObjectFileImpl(64, _64)

/**
 * Generates an implementation for the handling of symbol entries.
 *
 * @param bits the amount of bits to generate the implementation for
 * @param suffix the suffix to be used for the system provided data structures
 */
#define machoParser_handleSymbolImpl(bits, suffix)                                                                  \
static inline void machoParser_handleSymbol##bits(struct machoParser* self,                                         \
                                                  const struct nlist##suffix* entry) {                              \
    if (!self->private.parsingState.currentSymbol.has_value) {                                                      \
        throw(invalid, self, "Handling symbol without active symbol");                                              \
    }                                                                                                               \
    const char* value = self->private.stringTable + machoParser_swap(self, 32, entry->n_un.n_strx);                 \
    if (*value == '\0') {                                                                                           \
        self->private.parsingState.currentSymbol.value.length = machoParser_swap(self, bits, entry->n_value);       \
    } else {                                                                                                        \
        self->private.parsingState.currentSymbol.value.linkedName   = strdup(value);                                \
        self->private.parsingState.currentSymbol.value.startAddress = machoParser_swap(self, bits, entry->n_value); \
    }                                                                                                               \
}

machoParser_handleSymbolImpl(32,)
machoParser_handleSymbolImpl(64, _64)

/**
 * Generates an implementation for the handling of general entries.
 *
 * @param bits the amount of bits to generate the implementation for
 * @param suffix the suffix to be used for the system provided data structures
 */
#define machoParser_handleGeneralEntryImpl(bits, suffix)                                        \
static inline void machoParser_handleGeneralEntry##bits(struct machoParser* self,               \
                                                        const struct nlist##suffix* entry) {    \
    if ((entry->n_type & N_TYPE) != N_SECT) return;                                             \
                                                                                                \
    self->symbolCallback(self->object, (pair_symbolFile_t) {                                    \
        (struct symbol) {                                                                       \
            machoParser_swap(self, bits, entry->n_value),                                       \
            0,                                                                                  \
            strdup(self->private.stringTable + machoParser_swap(self, 32, entry->n_un.n_strx)), \
            { .has_value = false }                                                              \
        },                                                                                      \
        NULL                                                                                    \
    });                                                                                         \
}

machoParser_handleGeneralEntryImpl(32,)
machoParser_handleGeneralEntryImpl(64, _64)

/**
 * Generates an implementation for handling symbol table entries.
 *
 * @param bits the amount of bits to generate the implementation for
 * @param suffix the suffix to be used for the system provided data structures
 */
#define machoParser_handleEntryImpl(bits, suffix)                                     \
static inline void machoParser_handleEntry##bits(struct machoParser* self,            \
                                                 const struct nlist##suffix* entry) { \
    switch (entry->n_type) {                                                          \
        case N_BNSYM: machoParser_handleSymbolBegin##bits(self, entry); break;        \
        case N_ENSYM: machoParser_handleSymbolEnd##bits(self);          break;        \
                                                                                      \
        case N_SO:  machoParser_handleSourceInfo##bits(self, entry);    break;        \
        case N_OSO: machoParser_handleObjectFile##bits(self, entry);    break;        \
                                                                                      \
        case N_FUN: machoParser_handleSymbol##bits(self, entry);        break;        \
                                                                                      \
        default: machoParser_handleGeneralEntry##bits(self, entry);     break;        \
    }                                                                                 \
}

machoParser_handleEntryImpl(32,)
machoParser_handleEntryImpl(64, _64)

/**
 * Handles the given symbol table entry.
 *
 * @param self the Mach-O symbol table parser
 * @param entryAddress the address of the entry to be handled
 */
static inline void machoParser_handleEntry(struct machoParser* self, const void* entryAddress) {
    if (self->bit64) {
        machoParser_handleEntry64(self, entryAddress);
    } else {
        machoParser_handleEntry32(self, entryAddress);
    }
}

void machoParser_parseSymbolTable(struct machoParser* self) {
    const uint32_t symCnt  = machoParser_swap(self, 32, self->command->nsyms),
                   symOff = machoParser_swap(self, 32, self->command->symoff);
    for (uint32_t i = 0; i < symCnt; ++i) {
        machoParser_handleEntry(self, self->baseAddress + symOff + self->parsingOffset
                                      + i * self->private.entrySize);
    }
}

void machoParser_destroy(const struct machoParser* self) {
    if (self->private.parsingState.currentSymbol.has_value) {
        symbol_destroy(&self->private.parsingState.currentSymbol.value);
    }
}