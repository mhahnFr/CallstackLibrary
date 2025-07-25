#
# CallstackLibrary - Library creating human-readable call stacks.
#
# Copyright (C) 2022 - 2025  mhahnFr
#
# This file is part of the CallstackLibrary.
#
# The CallstackLibrary is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# The CallstackLibrary is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with the
# CallstackLibrary, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
#

CXX_FUNCTIONS ?= false
USE_BUILTINS  ?= true
INSTALL_PATH  ?= /usr/local

MACOS_ARCH_FLAGS =

ifdef MACOS_ARCH_FLAGS2
MACOS_ARCH_FLAGS = $(MACOS_ARCH_FLAGS2)
endif

# Library names
CORE_NAME = libcallstack
DYLIB_N   = $(CORE_NAME).dylib
SHARED_N  = $(CORE_NAME).so
STATIC_N  = $(CORE_NAME).a
# -------------

LD = $(CC)

# Assert submodules are available
ifeq ($(shell ls DC4C),)
	_  = $(shell git submodule init)
	_ += $(shell git submodule update)
endif
# -------------------------------

# Main sources
SRCS = \
	src/loadedLibInfo.c \
	src/callstackInternal.c \
	src/callstack_internals.c \
	src/callstack_frame.c \
	src/callstack.c \
	src/utils/file/pathUtils.c \
	src/regions/regions.c \
	src/parser/callstack_parser.c \
	src/parser/file/loader.c \
	src/parser/file/function.c \
	src/parser/file/bounds.c \
	src/parser/file/binaryFile.c \
	src/parser/file/dwarf/leb128.c \
	src/parser/file/dwarf/dwarf_parser.c \
	src/parser/file/dwarf/v5/parser.c \
	src/parser/file/dwarf/v4/parser.c \
	src/parser/demangling/demangler.c \
	src/parser/demangling/swift/demangler.c \
	src/functionInfo/functionInfo.c \
	src/dlMapper/dlMapper.c

OBJS = $(patsubst %.c, %.o, $(SRCS))
DEPS = $(patsubst %.c, %.d, $(SRCS))
# ------------

# C++ sources
CXX_SRCS = \
	src/parser/demangling/cxx/demangler.cpp \
	src/utils/file/fileHelper.cpp

CXX_OBJS = $(patsubst %.cpp, %.o, $(CXX_SRCS))
CXX_DEPS = $(patsubst %.cpp, %.d, $(CXX_SRCS))
# -----------

# Linux specific sources
LINUX_SRCS = \
	src/parser/file/elf/elfFile.c \
	src/dlMapper/elf/dlMapper.c

LINUX_OBJS  = $(patsubst %.c, %.o, $(LINUX_SRCS))
LINUX_DEPS  = $(patsubst %.c, %.d, $(LINUX_SRCS))
# ----------------------

# Darwin specific sources
DARWIN_SRCS = \
	src/utils/macho/fat_handler.c \
	src/parser/file/macho/archive.c \
	src/parser/file/macho/cache.c \
	src/parser/file/macho/macho_parser.c \
	src/parser/file/macho/machoFile.c \
	src/parser/file/macho/objectFile.c \
	src/dlMapper/macho/dlMapper.c

DARWIN_OBJS  = $(patsubst %.c, %.o, $(DARWIN_SRCS))
DARWIN_DEPS  = $(patsubst %.c, %.d, $(DARWIN_SRCS))
# -----------------------

# Compile and link flags
COM_FLAGS = -Wall -Wextra -fPIC -I DC4C -I 'include' -I src/utils
ifeq ($(USE_BUILTINS),true)
	COM_FLAGS += -DLCS_USE_BUILTINS
endif

LDFLAGS =
# ----------------------

NAME = $(STATIC_N)

ifdef MACOS_ARCH_FLAGS2
ifeq ($(CXX_FUNCTIONS),true)
MACOS_ARCH_FLAGS += -mmacosx-version-min=10.15
else
MACOS_ARCH_FLAGS += -mmacosx-version-min=10.6
endif
endif

ifeq ($(shell uname -s),Darwin)
	LDFLAGS   += -current_version 2.2 -compatibility_version 1 $(MACOS_ARCH_FLAGS)
	COM_FLAGS += $(MACOS_ARCH_FLAGS) -O3 -ffast-math
	OBJS      += $(DARWIN_OBJS)
	DEPS      += $(DARWIN_DEPS)

	NAME = $(DYLIB_N)
else ifeq ($(shell uname -s),Linux)
	OBJS += $(LINUX_OBJS)
	DEPS += $(LINUX_DEPS)
	COM_FLAGS += -Ofast

	NAME = $(SHARED_N)
else
$(error Unsupported platform)
endif

CFLAGS   = $(COM_FLAGS) -std=gnu11
CXXFLAGS = $(COM_FLAGS) -std=c++17

ifeq ($(CXX_FUNCTIONS),true)
	LD      = $(CXX)
	OBJS   += $(CXX_OBJS)
	CFLAGS += -DCXX_FUNCTIONS
	DEPS   += $(CXX_DEPS)
endif

default: $(NAME)

all:
	$(MAKE) $(SHARED_N)
	$(MAKE) $(STATIC_N)
	- $(MAKE) $(DYLIB_N)

release: clean
	$(MAKE) MACOS_ARCH_FLAGS2="-arch x86_64 -arch arm64 -arch arm64e" $(NAME) $(STATIC_N)

install: $(NAME)
	mkdir -p $(INSTALL_PATH)/lib
	mkdir -p "$(INSTALL_PATH)/include/CallstackLibrary"
	cp $(NAME) $(INSTALL_PATH)/lib
	find "include" \( -name \*.h -o -name \*.hpp \) -exec cp {} "$(INSTALL_PATH)/include/CallstackLibrary" \;

uninstall:
	- $(RM) $(INSTALL_PATH)/lib/$(NAME)
	- $(RM) -r "$(INSTALL_PATH)/include/CallstackLibrary"

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
	- $(RM) $(CXX_OBJS) $(CXX_DEPS)
	- $(RM) $(DYLIB_N) $(SHARED_N) $(STATIC_N)

re: clean
	$(MAKE) default

.PHONY: re clean all default install uninstall release

-include $(DEPS)
