 #
 #  Copyright (C) 2016, Zhang YanMing <jamincheung@126.com>
 #
 #  Linux recovery updater
 #
 #  This program is free software; you can redistribute it and/or modify it
 #  under  the terms of the GNU General  Public License as published by the
 #  Free Software Foundation;  either version 2 of the License, or (at your
 #  option) any later version.
 #
 #  You should have received a copy of the GNU General Public License along
 #  with this program; if not, write to the Free Software Foundation, Inc.,
 #  675 Mass Ave, Cambridge, MA 02139, USA.
 #
 #

SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
        else if [ -x /bin/bash ]; then echo /bin/bash; \
        else echo sh; fi; fi)

TARGET_NAME := ingenic
TARGET := lib$(TARGET_NAME).so

#
# Top directory
#
TOPDIR ?= $(shell pwd)

#
# Out & Tools directory
#
OUTDIR := $(TOPDIR)/out
$(shell [ -d $(OUTDIR) ] || mkdir -p $(OUTDIR))
OUTDIR := $(shell cd $(OUTDIR) && /bin/pwd)
$(if $(OUTDIR),,$(error output directory "$(OUTDIR)" does not exist))

#
# Cross compiler
#
CROSS_COMPILE ?= mips-linux-gnu-
CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
STRIP := $(CROSS_COMPILE)strip

#
# Compiler & Linker options
#
ARFLAGS := rcv
INCLUDES := -I$(TOPDIR)/include                                                \
            -I$(TOPDIR)/include/lib                                            \
            -I$(TOPDIR)/include/lib/zlib                                       \
            -I$(TOPDIR)/include/lib/zip/minizip

CFLAGS := -std=gnu11 $(INCLUDES) -fPIC
CHECKFLAGS := -Wall -Wuninitialized -Wundef
LDSHFLAGS := -shared -Wl,-Bsymbolic

#
# Library link - Static
#

LDFLAGS := -Wl,-Bstatic -L$(TOPDIR)/lib/fingerprint -lgoodix_fingerprint

#
# Library link - Dynamic
#
LDFLAGS += -Wl,-Bdynamic -pthread -lm -lrt -ldl                                \
           -L$(TOPDIR)/lib/openssl -lcrypto -lssl                              \
           -L$(TOPDIR)/lib/fingerprint -lfprint-mips

ifndef DEBUG
CFLAGS += -Os -mhard-float -D_GNU_SOURCE
else
CFLAGS += -D_GNU_SOURCE -g -DLOCAL_DEBUG -DDEBUG
endif

override CFLAGS := $(CHECKFLAGS) $(CFLAGS)

#
# Quiet compile
#
COMPILE_SRC := $(CC) $(CFLAGS) -c
LINK_OBJS   := $(CC) $(CFLAGS)

ifndef V
QUIET_AR        = @echo -e "  AR\t$@";
QUIET_CC        = @echo -e "  CC\t$@";
QUIET_LINK      = @echo -e "  LINK\t$@";

QUITE_TEST_BUILD = @echo -e "  BUILD\t"
QUITE_TEST_CLEAN = @echo -e "  CLEAN\t"
endif

%.o:%.c
	$(QUIET_CC) $(COMPILE_SRC) $< -o $@
