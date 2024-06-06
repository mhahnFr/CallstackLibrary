/*
 * Callstack Library - Library creating human-readable call stacks.
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

#ifndef loader_h
#define loader_h

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The simple parser callback function prototype.
 *
 * The first passed parameter is the additional payload, the second one is the loaded file buffer.
 *
 * Returns whether the parsing was successful.
 */
typedef bool (*loader_parser)(void*, void*);
/**
 * @brief The complex parser callback function prototype.
 *
 * The first passed parameter is the loaded file buffer, the second one the file name, the third one the
 * size of the file and the last parameter is the additional payload.
 *
 * Returns whether the parsing was successful.
 */
typedef bool (*loader_parserExtended)(void*, const char*, size_t, void*);

/**
 * Union consisting of the two possible parser functions.
 */
union loader_parserFunction {
    /** The simple parser function.  */
    loader_parser parseFunc;
    /** The complex parser function. */
    loader_parserExtended parseFuncExtended;
};

/**
 * @brief Loads the file of the given name and executes the given parser function using the payload and
 * the loaded data.
 *
 * The function can either be the simple one: It gets the payload as first argument and the file buffer as second
 * argument. The complex version gets the file buffer as first argument, the file name as second argument, the file's
 * size as third argument and the payload as last argument.
 *
 * The parser function is only called when the file was loaded successfully.
 *
 * @param fileName the name of the file to be loaded
 * @param func the parser function to be used
 * @param extended whether to use the complex version of the parser function
 * @param args the payload passed to the callback
 * @return whether the file was loaded successfully and the parsing was successful
 */
bool loader_loadFileAndExecute(const char* fileName, union loader_parserFunction func, bool extended, void* args);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* loader_h */
