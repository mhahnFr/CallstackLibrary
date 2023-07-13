#
# Callstack Library - Library creating human-readable call stacks.
#
# Copyright (C) 2022 - 2023  mhahnFr
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

# Library names
CORE_NAME = libcallstack
DYLIB_N   = $(CORE_NAME).dylib
SHARED_N  = $(CORE_NAME).so
STATIC_N  = $(CORE_NAME).a
# -------------

# Main sources
SRCS = code/callstack.c code/callstackInternal.c code/callstack_internals.c  \
       code/parser/callstack_parser.c code/parser/callstack_parserInternal.c \
       code/parser/file/binaryFile.c code/parser/file/cache/cache.c
OBJS = $(patsubst %.c, %.o, $(SRCS))
DEPS = $(patsubst %.c, %.d, $(SRCS))
# ------------

# C++ sources
CXX_SRCS = $(shell find . -type f -name \*.cpp)
CXX_OBJS = $(patsubst %.cpp, %.o, $(CXX_SRCS))
CXX_DEPS = $(patsubst %.cpp, %.d, $(CXX_SRCS))
# -----------

# Linux specific sources
LINUX_SRCS = $(shell find ./code/parser/file/elf -type f -name \*.c)
LINUX_OBJS = $(patsubst %.c, %.o, $(LINUX_SRCS))
LINUX_DEPS = $(patsubst %.c, %.d, $(LINUX_SRCS))
# ----------------------

# Darwin specific sources
DARWIN_SRCS = $(shell find ./code/parser/file/macho -type f -name \*.c)
DARWIN_OBJS = $(patsubst %.c, %.o, $(DARWIN_SRCS))
DARWIN_DEPS = $(patsubst %.c, %.d, $(DARWIN_SRCS))
# -----------------------

# Compile and link flags
COM_FLAGS = -Wall -Wextra -fPIC -Ofast
CFLAGS    = $(COM_FLAGS) -std=gnu11
CXXFLAGS  = $(COM_FLAGS) -std=gnu++11
LDFLAGS   = -ldl
# ----------------------

LD = $(CC)

NAME = $(STATIC_N)

ifeq ($(CXX_DEMANGLER),true)
	LD      = $(CXX)
	OBJS   += $(CXX_OBJS)
	CFLAGS += -DCXX_DEMANGLE
	DEPS   += $(CXX_DEPS)
endif

ifeq ($(shell uname -s),Darwin)
	LDFLAGS += -current_version 1.0.0 -compatibility_version 1 -install_name $(abspath $@)
	OBJS    += $(DARWIN_OBJS)
	DEPS    += $(DARWIN_DEPS)
else ifeq ($(shell uname -s),Linux)
	OBJS += $(LINUX_OBJS)
	DEPS += $(LINUX_DEPS)
else
$(error Unsupported platform)
endif

INSTALL_PATH ?= /usr/local

default: $(NAME)

all: $(SHARED_N) $(STATIC_N) $(DYLIB_N)

install: $(SHARED_N)
	mkdir -p $(INSTALL_PATH)/lib
	mkdir -p "$(INSTALL_PATH)/include"
	cp $(SHARED_N) $(INSTALL_PATH)/lib
	find "include" \( -name \*.h -o -name \*.hpp \) -exec cp {} "$(INSTALL_PATH)/include" \;

uninstall:
	- $(RM) $(INSTALL_PATH)/lib/$(SHARED_N)
	- $(RM) $(addprefix $(INSTALL_PATH)/, $(shell find "include" -name \*.h -o -name \*.hpp))

$(DYLIB_N): $(OBJS)
	$(LD) -dynamiclib $(LDFLAGS) -o $(DYLIB_N) $(OBJS)

$(SHARED_N): $(OBJS)
	$(LD) -shared -fPIC $(LDFLAGS) -o $(SHARED_N) $(OBJS)

$(STATIC_N): $(OBJS)
	$(AR) -crs $(STATIC_N) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

clean:
	- $(RM) $(OBJS) $(DEPS)
	- $(RM) $(CXX_OBJS)

fclean: clean
	- $(RM) $(DYLIB_N) $(SHARED_N) $(STATIC_N)

re: fclean
	$(MAKE) default

.PHONY: re fclean clean all default install uninstall

-include $(DEPS)
