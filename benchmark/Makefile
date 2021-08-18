# $Id$ 

ARCH := mips

MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
PROJECT := raptor-$(notdir $(patsubst %/,%,$(MAKEFILE_DIR)))

override TOPDIR_LNK="$(MAKEFILE_DIR)/.openwrt"
TOPDIR := $(shell [ -L $(TOPDIR_LNK) ] && readlink -f $(TOPDIR_LNK) 2>/dev/null)
ifeq ($(TOPDIR),)
	TOPDIR := /work/projects/openwrt/openwrt-18.06.9/git
endif

ifeq ($(ARCH),x86)
	COMMAND_PREFIX := ""
endif
ifeq ($(ARCH),mips)
	export STAGING_DIR := $(TOPDIR)/staging_dir
	COMMAND_PREFIX := $(STAGING_DIR)/toolchain-mips_24kc_gcc-7.3.0_musl/bin/mips-openwrt-linux-musl-
endif
ifeq ($(ARCH),)
	$(error ARCH not set)
endif
EXT := -$(ARCH)

BINARY := $(PROJECT)$(EXT)

# Where to copy the resulting executables
override INSTALL_LNK="$(MAKEFILE_DIR)/.install$(EXT)"
INSTALLDIR = $(shell readlink -f $(INSTALL_LNK) 2>/dev/null)

# build quietly by default.  For a verbose build, run "make V=1"
ifndef V
	QUIET := @
endif
MAKEFLAGS += --no-print-directory

export CC	:= $(COMMAND_PREFIX)gcc
export CPP	:= $(COMMAND_PREFIX)g++
export AR	:= $(COMMAND_PREFIX)ar
export STRIP	:= $(COMMAND_PREFIX)strip

source_dirs := . 
include_dirs := . ../src ../external/eigen3
obj_dir := obj$(EXT)

CFLAGS := -std=gnu++14 -O3 -DNDEBUG
CFLAGS += -Wall -Wno-unknown-pragmas
ifeq ($(ARCH),mips)
	CFLAGS += -Wno-int-in-bool-context
endif	
CFLAGS += -DREVISION="$(REVISION)$(MODIFIED) $(BRANCH)" 
CFLAGS += -DREVDATE="$(REVDATE)"

LIBS=-lpthread

LDFLAGS = -L$(STAGING_DIR)/target-mips_24kc_musl/usr/lib

SOURCES := $(wildcard   $(addsuffix /*.cpp, $(source_dirs) ) )
SOURCES += $(wildcard   $(addsuffix /*.c, $(source_dirs) ) )

OBJECTS := $(SOURCES:.c=.o)  
OBJECTS := $(OBJECTS:.cpp=.o)  
OBJECTS := $(addprefix $(obj_dir)/,$(notdir $(OBJECTS)))
VPATH := $(source_dirs) $(include_dirs) 

default: mips

.PHONY: mips
mips:
	$(MAKE) install ARCH=mips

.PHONY: mips-clean
mips-clean:
	$(MAKE) clean ARCH=mips

.PHONY: x86
x86:
	$(MAKE) install ARCH=x86

.PHONY: x86-clean
x86-clean:
	$(MAKE) clean ARCH=x86

	
	
install: $(BINARY)
	@echo Install $^ binaries to $(INSTALLDIR)
	$(QUIET) install -d $(INSTALLDIR); install $^ $(INSTALLDIR)
	

$(BINARY): $(BINARY)-unstripped
	@echo Strip $@ from $^..
	$(QUIET) $(STRIP) -o $(BINARY) $(BINARY)-unstripped

$(BINARY)-unstripped: $(obj_dir) .git.h $(OBJECTS) 
	@echo Create $(BINARY) $(REVISION)$(MODIFIED) $(BRANCH) $(REVDATE) 
	@echo Linking $@ from $^...
	$(QUIET)  $(CPP)  -o $@  $(OBJECTS)  $(LDFLAGS) $(LIBS) 


.PHONY: git
git: clean .git.h

.git.h:
	$(QUIET) cd $(MAKEFILE_DIR) && sh git.sh

.PHONY: clean
clean:
	$(QUIET) rm -f  $(BINARY) $(BINARY)-unstripped .git.h; rm -f -r $(obj_dir);

.PHONY: distclean
distclean:
	$(QUIET) rm -f  $(PROJECT)-x86* $(PROJECT)-mips* .git.h; rm -f -r obj*;



$(obj_dir):
	$(QUIET) mkdir $(obj_dir)

include $(wildcard $(obj_dir)/*.d) 


$(obj_dir)/%.o:%.cpp
	@echo Compiling $@ from $<..
	$(QUIET)  $(CPP) $(CFLAGS)  -c -MMD $(addprefix -I,$(include_dirs)) $< -o $@

$(obj_dir)/%.o:%.c
	@echo Compiling $@ from $<..
	$(QUIET) $(CC) $(CFLAGS)  -c -MMD $(addprefix -I,$(include_dirs)) $<  -o $@

