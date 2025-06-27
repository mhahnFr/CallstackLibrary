/*
 * CallstackLibrary - Library creating human-readable call stacks.
 *
 * Copyright (C) 2022 - 2025  mhahnFr
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

#ifndef __lcs_callstack_h
 #warning Wrong inclusion of "callstack.hpp" redirected to '#include "callstack.h"'!
 #include "callstack.h"

#else
 #ifndef __lcs_callstack_hpp
 #define __lcs_callstack_hpp

 #if __cplusplus >= 201103
  #include <system_error>
 #else
  #include <stdexcept>
 #endif

 #include "callstack_create.h"

 #include "callstack_cxx_compat.hpp"

/**
 * This namespace contains a wrapper class for the @code struct callstack@endcode.
 * It is needed to avoid name conflicts between the structure and the wrapper class.
 */
namespace lcs {
/**
 * @brief A wrapper class around the @code struct callstack@endcode.
 *
 * It provides the usual constructors and operator overloads. Additionally, it contains the
 * possibility to implicitly cast an object of this class to a pointer to the C structure.
 */
class callstack {
    /** A @c typedef for convenience. */
    typedef ::callstack struct_callstack;
    /** The original C structure.     */
    struct_callstack self;

    /**
     * Helper function to throw the appropriate exception.
     *
     * @throws std::system_error if compiled using C++11 or newer, a @c std::runtime_error otherwise
     */
    inline static void error() {
#if __cplusplus >= 201103
        throw std::system_error(errno, std::generic_category());
#else
        throw std::runtime_error("Backtrace invalid");
#endif
    }

public:
    /**
     * @brief A trivial default constructor.
     *
     * Zero-initializes the underlying C structure. If the given boolean value is @c true,
     * it is initialized using the function @c callstack_emplace().
     * Throws a @c std::runtime_error (or a @c std::system_error if compiled using C++11 or newer) if
     * @c emplace is set to @c true and the backtrace could not be created.
     *
     * @param emplace Whether to call @c callstack_emplace().
     * @throws std::system_error if the backtrace could not be created
     */
    inline explicit callstack(const bool emplace = true) {
        if (emplace) {
            if (!callstack_emplace(*this)) {
                error();
            }
        } else {
            callstack_create(*this);
        }
    }

    /**
     * @brief Constructs this object using the given stack address.
     *
     * Initializes the underlying C structure using the function @c callstack_emplaceWithAddress().
     * Throws a @c std::runtime_error (or a @c std::system_error if compiled using C++11 or newer) if
     * the backtrace could not be created.
     *
     * @param address The stack address after which frames are ignored.
     * @throws std::system_error if the backtrace could not be created
     */
    inline explicit callstack(void* address) {
        if (!callstack_emplaceWithAddress(*this, address)) {
            error();
        }
    }

    /**
     * @brief Constructs the underlying C structure with the given backtrace.
     *
     * if the given trace length is smaller than zero, a @c std::runtime_error
     * (or a @c std::system_error if compiled using C++11 or newer) is thrown.
     *
     * @param trace The backtrace.
     * @param length The length of the given backtrace.
     * @throws std::system_error if the trace length is smaller than @c 0
     */
    inline callstack(void** trace, const int length) {
        if (!callstack_emplaceWithBacktrace(*this, trace, length)) {
            error();
        }
    }

    inline callstack(const callstack& other) LCS_NOEXCEPT {
        callstack_create(*this);
        callstack_copy(*this, other);
    }

    /**
     * @brief Constructs a callstack object from the given C structure.
     *
     * @param cCallstack The C structure to be copied.
     */
    inline explicit callstack(const struct_callstack* cCallstack) LCS_NOEXCEPT {
        callstack_create(*this);
        callstack_copy(*this, cCallstack);
    }

    inline ~callstack() LCS_NOEXCEPT {
       callstack_destroy(*this);
    }

    inline callstack& operator=(const callstack& other) LCS_NOEXCEPT {
        if (&other != this) {
            callstack_copy(*this, other);
        }
        return *this;
    }

#if __cplusplus >= 201103
    inline callstack(callstack&& other) noexcept
        : self(std::move(other.self)) {
        callstack_create(other);
    }

    inline auto operator=(callstack&& other) noexcept -> callstack& {
        callstack_destroy(*this);
        self = std::move(other.self);
        callstack_create(other);
        return *this;
    }
#endif

    /**
     * Translates this callstack object.
     *
     * @param onlyBinaries whether to only deduct the names of the runtime images
     * @return @c this
     * @throws std::runtime_error if the translation failed
     */
    inline callstack& translate(const bool onlyBinaries = false) {
        if (onlyBinaries) {
            if (callstack_getBinaries(*this) == LCS_NULL) {
                throw std::runtime_error("LCS: Failed to translate the callstack (binaries only)");
            }
        } else {
            if (callstack_toArray(*this) == LCS_NULL) {
                throw std::runtime_error("LCS: Failed to translate the callstack");
            }
        }
        return *this;
    }

#ifdef LCS_USE_UNSAFE_OPTIMIZATION
    /**
     * @brief Translates this callstack object.
     *
     * Only the names of the runtime images are deducted. They are backed by
     * the cache of the library and only valid as long as the cache is.
     *
     * @return @c this
     * @throws std::runtime_error if the translation failed
     */
    inline callstack& translateBinariesCached() {
        if (callstack_getBinariesCached(*this) == LCS_NULL) {
            throw std::runtime_error("LCS: Failed to translate the callstack (cached binaries)");
        }
        return *this;
    }
#endif

    LCS_CONSTEXPR inline const callstack_frame* begin() const LCS_NOEXCEPT {
        return self.frames;
    }

    LCS_CONSTEXPR inline const callstack_frame* end() const LCS_NOEXCEPT {
        return self.frames + self.frameCount;
    }

    inline operator       struct_callstack*()       LCS_NOEXCEPT { return &self; }
    inline operator const struct_callstack*() const LCS_NOEXCEPT { return &self; }

    inline       struct_callstack* operator->()       LCS_NOEXCEPT { return &self; }
    inline const struct_callstack* operator->() const LCS_NOEXCEPT { return &self; }
};
}

 #endif /* __lcs_callstack_hpp */
#endif /* __lcs_callstack_h */
