# =============================================================================
# libmov
#
# 本地完整树（含 src/）：编 so + demo
# Full private tree (with src/): build the shared library and demo
#
# 公开/授权包（无 src/，仅有 lib/libmov.so）：只编 demo
# Public / licensed package (no src/, only lib/libmov.so): build demo only
#
#   make
#   make CROSS_COMPILE=mips-linux-uclibc-
#   make demo
#   make install PREFIX=/usr/local
#   make clean
# =============================================================================

CROSS_COMPILE ?=
CC      := $(CROSS_COMPILE)gcc
AR      := $(CROSS_COMPILE)ar
RANLIB  := $(CROSS_COMPILE)ranlib

LIB_NAME    := mov
MAJOR       := 1
MINOR       := 0
PATCH       := 0
SONAME      := lib$(LIB_NAME).so.$(MAJOR)
REALNAME    := lib$(LIB_NAME).so.$(MAJOR).$(MINOR).$(PATCH)
LINKNAME    := lib$(LIB_NAME).so

APIDIR      := api
PUBLIC_HDR  := $(APIDIR)/mov.h
SRCDIR      := src
LIBDIR      := lib
DEMODIR     := demo
BUILDDIR    := build
OBJDIR      := $(BUILDDIR)/obj

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

DEMO_SRC := $(DEMODIR)/reserved_moov_demo.c
DEMO_BIN := $(BUILDDIR)/reserved_moov_demo

# Prefer freshly built so; fall back to prebuilt under lib/
LINK_LIBDIR := $(if $(SRCS),$(BUILDDIR),$(LIBDIR))

CPPFLAGS := -I$(SRCDIR)
CFLAGS   ?= -O2 -g -Wall
CFLAGS   += -fPIC
LDFLAGS  += -shared -Wl,-soname,$(SONAME)

.PHONY: all shared static demo install clean dirs check_prebuilt

ifeq ($(strip $(SRCS)),)
all: check_prebuilt demo
shared: check_prebuilt
	@echo "No src/ — using prebuilt $(LIBDIR)/$(LINKNAME)"
else
all: shared
shared: dirs $(BUILDDIR)/$(REALNAME) $(BUILDDIR)/$(SONAME) $(BUILDDIR)/$(LINKNAME)
endif

static: dirs $(BUILDDIR)/lib$(LIB_NAME).a

demo: $(if $(SRCS),shared,check_prebuilt) $(DEMO_BIN)

check_prebuilt:
	@test -f $(LIBDIR)/$(LINKNAME) || test -f $(LIBDIR)/$(REALNAME) || \
		(echo "Missing prebuilt library under $(LIBDIR)/"; \
		 echo "Place libmov.so there, or restore src/ to build from source."; \
		 exit 1)
	@mkdir -p $(BUILDDIR)
	@if [ -f $(LIBDIR)/$(REALNAME) ]; then \
		cp -f $(LIBDIR)/$(REALNAME) $(BUILDDIR)/$(REALNAME); \
		ln -sfn $(REALNAME) $(BUILDDIR)/$(SONAME); \
		ln -sfn $(SONAME) $(BUILDDIR)/$(LINKNAME); \
	elif [ -f $(LIBDIR)/$(LINKNAME) ]; then \
		cp -f $(LIBDIR)/$(LINKNAME) $(BUILDDIR)/$(LINKNAME); \
	fi

dirs:
	@mkdir -p $(OBJDIR) $(BUILDDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/$(REALNAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@mkdir -p $(LIBDIR)
	cp -f $@ $(LIBDIR)/$(REALNAME)
	ln -sfn $(REALNAME) $(LIBDIR)/$(SONAME)
	ln -sfn $(SONAME) $(LIBDIR)/$(LINKNAME)

$(BUILDDIR)/$(SONAME): $(BUILDDIR)/$(REALNAME)
	ln -sfn $(REALNAME) $@

$(BUILDDIR)/$(LINKNAME): $(BUILDDIR)/$(SONAME)
	ln -sfn $(SONAME) $@

$(BUILDDIR)/lib$(LIB_NAME).a: $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(DEMO_BIN): $(DEMO_SRC)
	@mkdir -p $(BUILDDIR)
	$(CC) -O2 -g -Wall -I$(APIDIR) $< -o $@ \
		-L$(BUILDDIR) -L$(LIBDIR) -l$(LIB_NAME) -Wl,-rpath,'$$ORIGIN'

PREFIX ?= /usr/local
install: $(if $(SRCS),shared,check_prebuilt)
	install -d $(DESTDIR)$(PREFIX)/lib
	install -d $(DESTDIR)$(PREFIX)/include
	@if [ -f $(BUILDDIR)/$(REALNAME) ]; then \
		install -m 755 $(BUILDDIR)/$(REALNAME) $(DESTDIR)$(PREFIX)/lib/; \
		ln -sfn $(REALNAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME); \
		ln -sfn $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(LINKNAME); \
	else \
		install -m 755 $(LIBDIR)/$(LINKNAME) $(DESTDIR)$(PREFIX)/lib/; \
	fi
	install -m 644 $(PUBLIC_HDR) $(DESTDIR)$(PREFIX)/include/mov.h

clean:
	rm -rf $(BUILDDIR)

-include $(DEPS)
