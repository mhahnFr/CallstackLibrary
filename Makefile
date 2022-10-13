#
# Callstack Library - A library creating human readable call stacks.
#
# Copyright (C) 2022  mhahnFr
#
# This file is part of the CallstackLibrary. This library is free software:
# you can redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the Free Software Foundation,
# either version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
#

CXX_DEMANGLER = false

CORE_NAME = libcallstack
DYLIB_N   = $(CORE_NAME).dylib
SHARED_N  = $(CORE_NAME).so
STARIC_N  = $(CORE_NAME).a

SRCS      = $(shell find . -type f -name \*.c)
OBJS      = $(patsubst %.c, %.o, $(SRCS))
CXX_SRCS  = $(shell find . -type f -name \*.cpp)
CXX_OBJS  = $(patsubst %.cpp, %.opp, $(CXX_SRCS))
DEPS      = $(patsubst %.c, %.d, $(SRCS))

COM_FLAGS = -Wall -Wextra -fPIC -Ofast
CFLAGS    = $(COM_FLAGS) -std=gnu11
CXXFLAGS  = $(COM_FLAGS) -std=gnu++11
LDFLAGS   = -ldl

LD = $(CC)

NAME = $(STARIC_N)

ifeq ($(CXX_DEMANGLER),true)
	LD      = $(CXX)
	OBJS   += $(CXX_OBJS)
	CFLAGS += -DCXX_DEMANGLE
endif

default: $(NAME)

all: $(SHARED_N) $(STARIC_N) $(DYLIB_N)

$(DYLIB_N): $(OBJS)
	$(LD) -dynamiclib -fPIC $(LDFLAGS) -o $(DYLIB_N) $(OBJS)

$(SHARED_N): $(OBJS)
	$(LD) -shared -fPIC $(LDFLAGS) -o $(SHARED_N) $(OBJS)

$(STARIC_N): $(OBJS)
	$(AR) -crs $(STARIC_N) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

%.opp: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

clean:
	- $(RM) $(OBJS) $(DEPS)
	- $(RM) $(CXX_OBJS)

fclean: clean
	- $(RM) $(DYLIB_N) $(SHARED_N) $(STARIC_N)

re: fclean
	$(MAKE) default

.PHONY: re fclean clean all default

-include $(DEPS)
