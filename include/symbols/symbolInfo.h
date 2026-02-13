/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2025 - 2026  mhahnFr
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

#ifndef _lcs_symbols_symbolInfo_h
#define _lcs_symbols_symbolInfo_h

#ifndef __cplusplus
# include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The symbol information structure.
 *
 * For the time being, the @c callstack_frame structure is used.
 *
 * @since v2.3
 */
typedef struct callstack_frame SymbolInfo;

/**
 * Attempts to deduct the debug information for the symbol represented by the
 * given address.
 *
 * @param address the address to find the closest symbol for
 * @param info the symbol information structure to be filled, must not be @c NULL
 * @return whether a symbol was found
 * @since v2.3
 */
bool symbols_getInfo(const void* address, SymbolInfo* info);

#ifdef LCS_USE_UNSAFE_OPTIMIZATION
/**
 * @brief Attempts to deduct the debug information for the symbol represented
 * by the given address.
 *
 * The symbol information structure may be filled with cached values.
 *
 * @param address the address to find the closest symbol for
 * @param info the symbol information structure to be filled, must not be @c NULL
 * @return whether a symbol was found
 * @since v2.3
 */
bool symbols_getInfoCached(const void* address, SymbolInfo* info);
#endif

/**
 * Destroys the given symbol information structure.
 *
 * @param info the symbol information structure to be destroyed, must not be @c NULL
 */
void symbols_destroyInfo(const SymbolInfo* info);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _lcs_symbols_symbolInfo_h