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
 #warning Wrong inclusion of "callstack.hpp" redirected to "#include "callstack.h"!
 #include "callstack.h"

#else
 #ifndef callstack_hpp
 #define callstack_hpp

namespace cs {
    class callstack {
        ::callstack self;
        
    public:
        callstack(bool emplace = true) {
            if (emplace) {
                callstack_emplace(*this);
            }
        }
        
        callstack(void ** trace, size_t length) {
            callstack_emplaceWithBacktrace(*this, trace, length);
        }
        
        callstack(const callstack & other) {
            callstack_copy(*this, other);
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
        callstack(callstack &&) = default;
        
        callstack & operator=(callstack && other) {
            callstack_destroy(*this);
            self = std::move(other.self);
            return *this;
        }
 #endif
        
        operator ::callstack *()             { return &self; }
        operator const ::callstack *() const { return &self; }
    };
}

 #endif /* callstack_hpp */
#endif
