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

#include <stdio.h>
#include <string.h>

#include <mach-o/nlist.h>
#include <mach-o/stab.h>

#include <try_catch.h>

#include "macho_parser.h"

#include "cache.h"
#include "macho_parser_nlist.h"

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

bool macho_parseSymtab(struct symtab_command* command,
                       const void*            baseAddress,
                       uint64_t               offset,
                       bool                   bytesSwapped,
                       bool                   bit64,
                       macho_addObjectFile    objCb,
                       macho_addFunction      funCb,
                       ...) {
    if (objCb == NULL && funCb == NULL) return false;
    
    const char*    stringBegin = baseAddress + (macho_maybeSwap(32, bytesSwapped, command->stroff) + offset);
    const uint32_t nsyms       = macho_maybeSwap(32, bytesSwapped, command->nsyms),
                   symoff      = macho_maybeSwap(32, bytesSwapped, command->symoff);
    
    va_list args;
    va_start(args, funCb);
    
    const char*              path           = NULL;
    const char*              sourceFileName = NULL;
    struct optional_function currFun        = { .has_value = false };
    struct objectFile*       currObj        = NULL;

#define ENSURE(condition, message) if (!(condition)) THROW(message)

    TRY({
        for (uint32_t i = 0; i < nsyms; ++i) {
            struct macho_parser_nlist entry = macho_parser_nlist_from(baseAddress + symoff + offset + i * macho_parser_nlist_sizeof(bit64), bit64, bytesSwapped);
            switch (entry.n_type) {
                case N_BNSYM:
                    ENSURE(!currFun.has_value, "N_BNSYM with active function");

                    function_create(&currFun.value);
                    currFun.has_value = true;
                    currFun.value.startAddress = entry.n_value;
                    break;

                case N_ENSYM:
                    ENSURE(currFun.has_value, "N_ENSYM without active function");

                    if (funCb != NULL) {
                        va_list copy;
                        va_copy(copy, args);
                        funCb((pair_funcFile_t) { currFun.value, currObj }, copy);
                        va_end(copy);
                    } else {
                        function_destroy(&currFun.value);
                    }
                    currFun.has_value = false;
                    break;

                case N_SO: {
                    const char* value = stringBegin + entry.n_strx;
                    if (*value == '\0') {
                        if (currObj == NULL) {
                            // Beginning, ignore
                            continue;
                        }
                        if (objCb != NULL) {
                            va_list copy;
                            va_copy(copy, args);
                            objCb(currObj, copy);
                            va_end(copy);
                        }
                        currObj = NULL;
                        path = NULL;
                        sourceFileName = NULL;
                    } else if (path == NULL) {
                        path = value;
                    } else {
                        sourceFileName = value;
                    }
                    break;
                }

                case N_OSO: {
                    ENSURE(currObj == NULL, "N_OSO with active object file");

                    const char*    fileName     = stringBegin + entry.n_strx;
                    const uint64_t lastModified = entry.n_value;

                    ENSURE((currObj = macho_cache_findOrAdd(fileName, lastModified)) != NULL,
                           "N_OSO: object file not found");
                    if (currObj->directory == NULL) {
                        currObj->directory = path == NULL ? NULL : strdup(path);
                    }
                    if (currObj->sourceFile == NULL) {
                        currObj->sourceFile = sourceFileName == NULL ? NULL : strdup(sourceFileName);
                    }
                    break;
                }

                case N_FUN: {
                    ENSURE(currFun.has_value, "N_FUN without active function");

                    const char* value = stringBegin + entry.n_strx;
                    if (*value == '\0') {
                        currFun.value.length = entry.n_value;
                    } else {
                        currFun.value.linkedName   = strdup(value);
                        currFun.value.startAddress = entry.n_value;
                    }
                    break;
                }

                default:
                    if (funCb != NULL && (entry.n_type & N_TYPE) == N_SECT) {
                        va_list copy;
                        va_copy(copy, args);
                        funCb((pair_funcFile_t) {
                            (struct function) {
                                .linkedName = strdup(stringBegin + entry.n_strx),
                                .startAddress = entry.n_value,
                                .length = 0x0,
                                .demangledName.has_value = false,
                            }, NULL
                        }, copy);
                        va_end(copy);
                    }
                    break;
            }
        }
    }, CATCH(const char*, message, {
        va_end(args);
        if (currFun.has_value) {
            function_destroy(&currFun.value);
        }
        printf("Failed to parse Mach-O: %s\n", message);
        return false;
    }))
    va_end(args);
    return true;
}
