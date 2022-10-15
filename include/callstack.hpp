/*
 * Callstack Library - A library creating human readable call stacks.
 *
 * Copyright (C) 2022  mhahnFr
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

#ifndef callstack_h
 #warning Wrong inclusion of "callstack.hpp" redirected to '#include "callstack.h"'!
 #include "callstack.h"

#else
 #ifndef callstack_hpp
 #define callstack_hpp

 #include "callstack_create.h"

 #if __cplusplus >= 201103
  #include <system_error>
 #else
  #include <stdexcept>
 #endif

/**
 * This namespace contains a wrapper class for the struct callstack.
 * It is needed to avoid name conflicts between the struct and the wrapper class.
 */
namespace lcs {
    /**
     * @brief A wrapper class around the struct callstack.
     *
     * It provides the usual constructors and operator overloads. Additionally, it contains the
     * possibility to implicitly cast an object of this class to a pointer to the C struct.
     */
    class callstack {
        /** A typedef for convenience. */
        typedef ::callstack struct_callstack;
        /** The original C struct.     */
        struct_callstack self;
        
        /**
         * @brief Helper function to throw the appopriate exception.
         *
         * @throws A system_error if compiled using C++11 or newer, a runtime error otherwise.
         */
        void error() {
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
         * Zero-initializes the underlying C struct. If the given bool value is true,
         * it is initialized using the function callstack_emplace().
         * Throws a runtime_error or a system_error if compiled using C++11 or newer if
         * emplace is set to true and the backtrace could not be created.
         *
         * @param emplace Whether to call callstack_emplace().
         */
        explicit callstack(bool emplace = true) {
            if (emplace) {
                if (!callstack_emplaceWithAddress(*this, __builtin_return_address(0))) {
                    error();
                }
            } else {
                callstack_create(*this);
            }
        }
        
        /**
         * @brief Constructs this object using the given stack address.
         *
         * Initializes the underlying C struct using the function callstack_emplaceWithAddress().
         * Throws a runtime_error or a system_error if compiled using C++11 or newer if
         * the backtrace could not be created.
         *
         * @param address The stack address after which frames are ignored.
         */
        explicit callstack(void * address) {
            if (!callstack_emplaceWithAddress(*this, address)) {
                error();
            }
        }
        
        /**
         * @brief Constructs the underlying C struct with the given backtrace.
         *
         * if the given trace length is smaller than zero, a runtime_error or a system_error
         * if compiled using C++11 or newer is thrown.
         *
         * @param trace The backtrace.
         * @param length The length of the given backtrace.
         */
        callstack(void ** trace, int length) {
            if (!callstack_emplaceWithBacktrace(*this, trace, length)) {
                error();
            }
        }
        
        callstack(const callstack & other) {
            callstack_create(*this);
            callstack_copy(*this, other);
        }
        
        /**
         * @brief Constructs a callstack object from the given C struct.
         *
         * @param cCallstack The C struct to be copied.
         */
        explicit callstack(const struct_callstack * cCallstack) {
            callstack_create(*this);
            callstack_copy(*this, cCallstack);
        }
        
       ~callstack() {
           callstack_destroy(*this);
        }
        
        callstack & operator=(const callstack & other) {
            if (&other != this) {
                callstack_copy(*this, other);
            }
            return *this;
        }
        
 #if __cplusplus >= 201103
        callstack(callstack && other)
            : self(std::move(other.self)) {
            callstack_create(other);
        }
        
        callstack & operator=(callstack && other) {
            callstack_destroy(*this);
            self = std::move(other.self);
            callstack_create(other);
            return *this;
        }
 #endif
        
        operator       struct_callstack * ()       { return &self; }
        operator const struct_callstack * () const { return &self; }
    };
}

 #endif /* callstack_hpp */
#endif /* callstack_h */
