/*
 * Callstack Library - Library creating human-readable call stacks.
 *
 * Copyright (C) 2023  mhahnFr
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

#ifndef callstack_exception_hpp
#define callstack_exception_hpp

#include <exception>
#include <string>

#include "callstack.h"

namespace lcs {
class exception: public std::exception {
    const std::string message;
    mutable callstack cs;
    
public:
    exception(): std::exception(), message(), cs(__builtin_return_address(0)) {}
    
    exception(const std::string & message)
        : std::exception(), message(message), cs(__builtin_return_address(0)) {}
    
    exception(const exception &) = default;
   ~exception() = default;
    
    // TODO: C++11 Move constructors
    
    virtual const char * what(); // TODO: Override nothrow
    
    callstack & getCallstack() const; // TODO: constexpr noexcept
    const std::string & getMessage() const; // TODO: constexpr noexcept
};
}

#endif /* callstack_exception_hpp */
