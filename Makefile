# SPDX-License-Identifier: ISC
#
# Copyright (C) 2024 Michael Drake <tlsa@netsurf-browser.org>

VARIANT = release

VALID_VARIANTS := release debug
ifneq ($(filter $(VARIANT),$(VALID_VARIANTS)),)
else
$(error Invalid VARIANT specified. Valid values are: $(VALID_VARIANTS))
endif

LIB_NAME = libcgifh
LIB_PKGCON = $(LIB_NAME).pc
LIB_STATIC = $(LIB_NAME).a
LIB_VERSION = 0.0.1

.IMPLICIT =

PREFIX ?= /usr/local
LIBDIR ?= lib
INCLUDEDIR ?= include

Q ?= @

CC ?= gcc
AR ?= ar
MKDIR =	mkdir -p
INSTALL ?= install -c -p

INCLUDE = -I include
CPPFLAGS += -MMD -MP
CFLAGS += $(INCLUDE)
CFLAGS += -std=c11 -Wall -Wextra -pedantic \
		-Wconversion -Wwrite-strings -Wcast-align -Wpointer-arith \
		-Winit-self -Wshadow -Wstrict-prototypes -Wmissing-prototypes \
		-Wredundant-decls -Wundef -Wvla -Wdeclaration-after-statement
LDFLAGS +=

ifeq ($(VARIANT), debug)
	CFLAGS += -O0 -g
else
	CFLAGS += -O3 -DNDEBUG
endif

BUILDDIR = build/$(VARIANT)

LIB_SRC_FILES = cgifh.c font.c

LIB_SRC = $(addprefix src/,$(LIB_SRC_FILES))
LIB_OBJ = $(patsubst %.c,%.o, $(addprefix $(BUILDDIR)/,$(LIB_SRC)))
LIB_DEP = $(patsubst %.c,%.d, $(addprefix $(BUILDDIR)/,$(LIB_SRC)))

all: $(BUILDDIR)/$(LIB_STATIC)

$(BUILDDIR)/$(LIB_PKGCON): $(LIB_PKGCON).in
	sed \
		-e 's#SED_PREFIX#$(PREFIX)#' \
		-e 's#SED_LIBDIR#$(LIBDIR)#' \
		-e 's#SED_INCLUDEDIR#$(INCLUDEDIR)#' \
		-e 's#SED_VERSION#$(LIB_VERSION)#' \
		$(LIB_PKGCON).in >$(BUILDDIR)/$(LIB_PKGCON)

$(BUILDDIR)/$(LIB_STATIC): $(LIB_OBJ)
	$(AR) -rcs $@ $^

$(LIB_OBJ): $(BUILDDIR)/%.o : %.c
	$(Q)$(MKDIR) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_COV) -c -o $@ $<

docs:
	$(MKDIR) build/docs/api
	$(MKDIR) build/docs/devel
	doxygen docs/api.doxygen.conf
	doxygen docs/devel.doxygen.conf

clean:
	rm -rf build/

install: $(BUILDDIR)/$(LIB_STATIC) $(BUILDDIR)/$(LIB_PKGCON)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/$(LIBDIR)
	$(INSTALL) $(BUILDDIR)/$(LIB_STATIC) $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(LIB_STATIC)
	chmod 644 $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(LIB_STATIC)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/$(INCLUDEDIR)
	$(INSTALL) -m 644 include/* $(DESTDIR)$(PREFIX)/$(INCLUDEDIR)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/$(LIBDIR)/pkgconfig
	$(INSTALL) -m 644 $(BUILDDIR)/$(LIB_PKGCON) $(DESTDIR)$(PREFIX)/$(LIBDIR)/pkgconfig/$(LIB_PKGCON)

-include $(LIB_DEP)

.PHONY: all clean docs install
