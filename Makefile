#
# CallstackLibrary - Library creating human-readable call stacks.
#
# Copyright (C) 2022 - 2024  mhahnFr
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

CXX_FUNCTIONS = false
USE_BUILTINS  = true

MACOS_ARCH_FLAGS =

# Library names
CORE_NAME = libcallstack
DYLIB_N   = $(CORE_NAME).dylib
SHARED_N  = $(CORE_NAME).so
STATIC_N  = $(CORE_NAME).a
# -------------

LD = $(CC)

# Paths
LINUX_PATH     = ./src/parser/file/elf
DARWIN_PATH    = ./src/parser/file/macho

DL_MAPPER_LINUX_PATH  = ./src/dlMapper/elf
DL_MAPPER_DARWIN_PATH = ./src/dlMapper/macho

UTILS_DARWIN_PATH = ./src/utils/macho
# -----

# Assert submodules are available
ifeq ($(shell ls DC4C),)
	_  = $(shell git submodule init)
	_ += $(shell git submodule update)
endif
# -------------------------------

# Main sources
SRCS = $(shell find ./src -type f -name \*.c \! -path $(LINUX_PATH)\* \! -path $(DARWIN_PATH)\* \! -path $(DL_MAPPER_LINUX_PATH)\* \! -path $(DL_MAPPER_DARWIN_PATH)\* \! -path $(UTILS_DARWIN_PATH)\*)
OBJS = $(patsubst %.c, %.o, $(SRCS))
DEPS = $(patsubst %.c, %.d, $(SRCS))
# ------------

# C++ sources
CXX_SRCS = $(shell find ./src -type f -name \*.cpp)
CXX_OBJS = $(patsubst %.cpp, %.o, $(CXX_SRCS))
CXX_DEPS = $(patsubst %.cpp, %.d, $(CXX_SRCS))
# -----------

# Linux specific sources
LINUX_SRCS  = $(shell find $(LINUX_PATH) -type f -name \*.c)
LINUX_SRCS += $(shell find $(DL_MAPPER_LINUX_PATH) -type f -name \*.c)
LINUX_OBJS  = $(patsubst %.c, %.o, $(LINUX_SRCS))
LINUX_DEPS  = $(patsubst %.c, %.d, $(LINUX_SRCS))
# ----------------------

# Darwin specific sources
DARWIN_SRCS  = $(shell find $(DARWIN_PATH) -type f -name \*.c)
DARWIN_SRCS += $(shell find $(DL_MAPPER_DARWIN_PATH) -type f -name \*.c)
DARWIN_SRCS += $(shell find $(UTILS_DARWIN_PATH) -type f -name \*.c)
DARWIN_OBJS  = $(patsubst %.c, %.o, $(DARWIN_SRCS))
DARWIN_DEPS  = $(patsubst %.c, %.d, $(DARWIN_SRCS))
# -----------------------

# Compile and link flags
COM_FLAGS = -Wall -Wextra -fPIC -Ofast -I DC4C -I 'include' -I src/utils
ifeq ($(USE_BUILTINS),true)
	COM_FLAGS += -DLCS_USE_BUILTINS
endif

LDFLAGS =
# ----------------------

NAME = $(STATIC_N)

ifeq ($(shell uname -s),Darwin)
	LDFLAGS   += -current_version 2.0.1 -compatibility_version 1 $(MACOS_ARCH_FLAGS)
	COM_FLAGS += $(MACOS_ARCH_FLAGS)
	OBJS      += $(DARWIN_OBJS)
	DEPS      += $(DARWIN_DEPS)

	NAME = $(DYLIB_N)
else ifeq ($(shell uname -s),Linux)
	OBJS += $(LINUX_OBJS)
	DEPS += $(LINUX_DEPS)

	NAME = $(SHARED_N)
else
$(error Unsupported platform)
endif

CFLAGS   = $(COM_FLAGS) -std=gnu11
CXXFLAGS = $(COM_FLAGS) -std=gnu++17

ifeq ($(CXX_FUNCTIONS),true)
	LD      = $(CXX)
	OBJS   += $(CXX_OBJS)
	CFLAGS += -DCXX_FUNCTIONS
	DEPS   += $(CXX_DEPS)
endif

INSTALL_PATH ?= /usr/local

default: $(NAME)

all:
	$(MAKE) $(SHARED_N)
	$(MAKE) $(STATIC_N)
	- $(MAKE) $(DYLIB_N)

release: fclean
	$(MAKE) MACOS_ARCH_FLAGS="-arch x86_64 -arch arm64 -arch arm64e" $(NAME) $(STATIC_N)

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

fclean: clean
	- $(RM) $(DYLIB_N) $(SHARED_N) $(STATIC_N)

re: fclean
	$(MAKE) default

.PHONY: re fclean clean all default install uninstall release

-include $(DEPS)
