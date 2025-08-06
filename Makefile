# SPDX-License-Identifier: GPL-2.0-or-later
#
# Device Tree Compiler
#

$(warning WARNING: Building dtc using make is deprecated, in favour of using Meson (https://mesonbuild.com))
$(warning )
$(warning Use `meson setup builddir/ && meson compile -C builddir/` to build, `meson test -C builddir/` to test, or `meson configure` to see build options.)

#
# Version information will be constructed in this order:
# DTC_VERSION release version as MAJOR.MINOR.PATCH
# LOCAL_VERSION is likely from command line.
# CONFIG_LOCALVERSION from some future config system.
#
DTC_VERSION = $(shell cat VERSION.txt)
LOCAL_VERSION =
CONFIG_LOCALVERSION =

# Control the assumptions made (e.g. risking security issues) in the code.
# See libfdt_internal.h for details
ASSUME_MASK ?= 0

CPPFLAGS = -I libfdt -I . -DFDT_ASSUME_MASK=$(ASSUME_MASK)
WARNINGS = -Wall -Wpointer-arith -Wcast-qual -Wnested-externs -Wsign-compare \
	-Wstrict-prototypes -Wmissing-prototypes -Wredundant-decls -Wshadow \
	-Wwrite-strings
ifeq ($(shell $(CC) --version | grep -q gcc && echo gcc),gcc)
WARNINGS += -Wsuggest-attribute=format
endif
CFLAGS = -g -Os $(SHAREDLIB_CFLAGS) -Werror $(WARNINGS) $(EXTRA_CFLAGS)

BISON = bison
LEX = flex
SWIG = swig
PKG_CONFIG ?= pkg-config

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_LIB = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644
INSTALL_SCRIPT = $(INSTALL)
DESTDIR =
PREFIX = $(HOME)
BINDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

HOSTOS := $(shell uname -s | tr '[:upper:]' '[:lower:]' | \
	    sed -e 's/\(cygwin\|msys\).*/\1/')

NO_VALGRIND := $(shell $(PKG_CONFIG) --exists valgrind; echo $$?)
ifeq ($(NO_VALGRIND),1)
	CPPFLAGS += -DNO_VALGRIND
else
	CFLAGS += $(shell $(PKG_CONFIG) --cflags valgrind)
endif

# libyaml before version 0.2.3 expects non-const string parameters. Supporting
# both variants would require either cpp magic or passing
# -Wno-error=discarded-qualifiers to the compiler. For the sake of simplicity
# just support libyaml >= 0.2.3.
NO_YAML := $(shell $(PKG_CONFIG) --atleast-version 0.2.3 yaml-0.1; echo $$?)
ifeq ($(NO_YAML),1)
	CFLAGS += -DNO_YAML
else
	LDLIBS_dtc += $(shell $(PKG_CONFIG) --libs yaml-0.1)
	CFLAGS += $(shell $(PKG_CONFIG) --cflags yaml-0.1)
endif

HAS_VERSION_SCRIPT := $(shell printf 'void test_func(){}\nint main(){}' | $(CC) -Wl,--version-script=libfdt/test.lds -x c - && echo y || echo n)

ifeq ($(HOSTOS),darwin)
SHAREDLIB_EXT     = dylib
SHAREDLIB_CFLAGS  = -fPIC
SHAREDLIB_LDFLAGS = -fPIC -dynamiclib -Wl,-install_name -Wl,
else ifeq ($(HOSTOS),$(filter $(HOSTOS),msys cygwin))
SHAREDLIB_EXT     = so
SHAREDLIB_CFLAGS  =
SHAREDLIB_LDFLAGS = -shared -Wl,-soname,
else
SHAREDLIB_EXT     = so
SHAREDLIB_CFLAGS  = -fPIC
SHAREDLIB_LDFLAGS = -fPIC -shared -Wl,-soname,
endif

ifeq ($(HAS_VERSION_SCRIPT),y)
SHAREDLIB_LDFLAGS += -Wl,--version-script=$(LIBFDT_version)
endif

#
# Overall rules
#
ifdef V
VECHO = :
else
VECHO = echo "	"
ARFLAGS = rc
.SILENT:
endif

NODEPTARGETS = clean
ifeq ($(MAKECMDGOALS),)
DEPTARGETS = all
else
DEPTARGETS = $(filter-out $(NODEPTARGETS),$(MAKECMDGOALS))
endif

#
# Rules for versioning
#

VERSION_FILE = version_gen.h

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

nullstring :=
space	:= $(nullstring) # end of line

localver_config = $(subst $(space),, $(string) \
			      $(patsubst "%",%,$(CONFIG_LOCALVERSION)))

localver_cmd = $(subst $(space),, $(string) \
			      $(patsubst "%",%,$(LOCALVERSION)))

localver_scm = $(shell $(CONFIG_SHELL) ./scripts/setlocalversion)
localver_full  = $(localver_config)$(localver_cmd)$(localver_scm)

dtc_version = $(DTC_VERSION)$(localver_full)

# Contents of the generated version file.
define filechk_version
	(echo "#define DTC_VERSION \"DTC $(dtc_version)\""; )
endef

define filechk
	set -e;					\
	echo '	CHK $@';			\
	mkdir -p $(dir $@);			\
	$(filechk_$(1)) < $< > $@.tmp;		\
	if [ -r $@ ] && cmp -s $@ $@.tmp; then	\
		rm -f $@.tmp;			\
	else					\
		echo '	UPD $@';		\
		mv -f $@.tmp $@;		\
	fi;
endef


include Makefile.convert-dtsv0
include Makefile.dtc
include Makefile.utils

BIN += convert-dtsv0
BIN += dtc
BIN += fdtdump
BIN += fdtget
BIN += fdtput
BIN += fdtoverlay

SCRIPTS = dtdiff

all: $(BIN) libfdt

ifneq ($(DEPTARGETS),)
ifneq ($(MAKECMDGOALS),libfdt)
-include $(DTC_OBJS:%.o=%.d)
-include $(CONVERT_OBJS:%.o=%.d)
-include $(FDTDUMP_OBJS:%.o=%.d)
-include $(FDTGET_OBJS:%.o=%.d)
-include $(FDTPUT_OBJS:%.o=%.d)
-include $(FDTOVERLAY_OBJS:%.o=%.d)
endif
endif



#
# Rules for libfdt
#
LIBFDT_dir = libfdt
LIBFDT_archive = $(LIBFDT_dir)/libfdt.a
LIBFDT_lib = $(LIBFDT_dir)/$(LIBFDT_LIB)
LIBFDT_include = $(addprefix $(LIBFDT_dir)/,$(LIBFDT_INCLUDES))
LIBFDT_version = $(addprefix $(LIBFDT_dir)/,$(LIBFDT_VERSION))

ifeq ($(STATIC_BUILD),1)
	CFLAGS += -static
	LIBFDT_dep = $(LIBFDT_archive)
else
	LIBFDT_dep = $(LIBFDT_lib)
endif

include $(LIBFDT_dir)/Makefile.libfdt

.PHONY: libfdt
libfdt: $(LIBFDT_archive) $(LIBFDT_lib)

$(LIBFDT_archive): $(addprefix $(LIBFDT_dir)/,$(LIBFDT_OBJS))

$(LIBFDT_lib): $(addprefix $(LIBFDT_dir)/,$(LIBFDT_OBJS)) $(LIBFDT_version)
	@$(VECHO) LD $@
	$(CC) $(LDFLAGS) $(SHAREDLIB_LDFLAGS)$(LIBFDT_soname) -o $(LIBFDT_lib) \
		$(addprefix $(LIBFDT_dir)/,$(LIBFDT_OBJS))
	ln -sf $(LIBFDT_LIB) $(LIBFDT_dir)/$(LIBFDT_soname)
	ln -sf $(LIBFDT_soname) $(LIBFDT_dir)/$(LIBFDT_so)

ifneq ($(DEPTARGETS),)
-include $(LIBFDT_OBJS:%.o=$(LIBFDT_dir)/%.d)
endif

# This stops make from generating the lex and bison output during
# auto-dependency computation, but throwing them away as an
# intermediate target and building them again "for real"
.SECONDARY: $(DTC_GEN_SRCS) $(CONVERT_GEN_SRCS)

install-bin: all $(SCRIPTS)
	@$(VECHO) INSTALL-BIN
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL_PROGRAM) $(BIN) $(DESTDIR)$(BINDIR)
	$(INSTALL_SCRIPT) $(SCRIPTS) $(DESTDIR)$(BINDIR)

install-lib: libfdt
	@$(VECHO) INSTALL-LIB
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(INSTALL_LIB) $(LIBFDT_lib) $(DESTDIR)$(LIBDIR)
	ln -sf $(notdir $(LIBFDT_lib)) $(DESTDIR)$(LIBDIR)/$(LIBFDT_soname)
	ln -sf $(LIBFDT_soname) $(DESTDIR)$(LIBDIR)/libfdt.$(SHAREDLIB_EXT)
	$(INSTALL_DATA) $(LIBFDT_archive) $(DESTDIR)$(LIBDIR)

install-includes:
	@$(VECHO) INSTALL-INC
	$(INSTALL) -d $(DESTDIR)$(INCLUDEDIR)
	$(INSTALL_DATA) $(LIBFDT_include) $(DESTDIR)$(INCLUDEDIR)

install: install-bin install-lib install-includes

$(VERSION_FILE): Makefile FORCE
	$(call filechk,version)


dtc: $(DTC_OBJS)

convert-dtsv0: $(CONVERT_OBJS)
	@$(VECHO) LD $@
	$(LINK.c) -o $@ $^

fdtdump:	$(FDTDUMP_OBJS)

fdtget:	$(FDTGET_OBJS) $(LIBFDT_dep)

fdtput:	$(FDTPUT_OBJS) $(LIBFDT_dep)

fdtoverlay: $(FDTOVERLAY_OBJS) $(LIBFDT_dep)

dist:
	git archive --format=tar --prefix=dtc-$(dtc_version)/ HEAD \
		> ../dtc-$(dtc_version).tar
	cat ../dtc-$(dtc_version).tar | \
		gzip -9 > ../dtc-$(dtc_version).tar.gz


#
# Release signing and uploading
# This is for maintainer convenience, don't try this at home.
#
ifeq ($(MAINTAINER),y)
GPG = gpg2
KUP = kup
KUPDIR = /pub/software/utils/dtc

kup: dist
	$(GPG) --detach-sign --armor -o ../dtc-$(dtc_version).tar.sign \
		../dtc-$(dtc_version).tar
	$(KUP) put ../dtc-$(dtc_version).tar.gz ../dtc-$(dtc_version).tar.sign \
		$(KUPDIR)/dtc-$(dtc_version).tar.gz
endif

tags: FORCE
	rm -f tags
	find . \( -name tests -type d -prune \) -o \
	       \( ! -name '*.tab.[ch]' ! -name '*.lex.c' \
	       -name '*.[chly]' -type f -print \) | xargs ctags -a

#
# Testsuite rules
#
TESTS_PREFIX=tests/

TESTS_BIN += dtc
TESTS_BIN += convert-dtsv0
TESTS_BIN += fdtput
TESTS_BIN += fdtget
TESTS_BIN += fdtdump
TESTS_BIN += fdtoverlay

ifneq ($(MAKECMDGOALS),libfdt)
include tests/Makefile.tests
endif

#
# Clean rules
#
STD_CLEANFILES = *~ *.o *.$(SHAREDLIB_EXT) *.d *.a *.i *.s core a.out vgcore.* \
	*.tab.[ch] *.lex.c *.output

clean: libfdt_clean tests_clean
	@$(VECHO) CLEAN
	rm -f $(STD_CLEANFILES)
	rm -f $(VERSION_FILE)
	rm -f $(BIN)
	rm -f dtc-*.tar dtc-*.tar.sign dtc-*.tar.asc

#
# Generic compile rules
#
%: %.o
	@$(VECHO) LD $@
	$(LINK.c) -o $@ $^ $(LDLIBS_$*)

%.o: %.c
	@$(VECHO) CC $@
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

%.o: %.S
	@$(VECHO) AS $@
	$(CC) $(CPPFLAGS) $(AFLAGS) -o $@ -c $<

%.d: %.c
	@$(VECHO) DEP $<
	$(CC) $(CPPFLAGS) $(CFLAGS) -MM -MG -MT "$*.o $@" $< > $@

%.d: %.S
	@$(VECHO) DEP $<
	$(CC) $(CPPFLAGS) -MM -MG -MT "$*.o $@" $< > $@

%.i:	%.c
	@$(VECHO) CPP $@
	$(CC) $(CPPFLAGS) -E $< > $@

%.s:	%.c
	@$(VECHO) CC -S $@
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -S $<

%.a:
	@$(VECHO) AR $@
	$(AR) $(ARFLAGS) $@ $^

%.lex.c: %.l
	@$(VECHO) LEX $@
	$(LEX) -o$@ $<

%.tab.c %.tab.h: %.y
	@$(VECHO) BISON $@
	$(BISON) -b $(basename $(basename $@)) -d $<

FORCE:

ifeq ($(MAKE_RESTARTS),10)
$(error "Make re-executed itself $(MAKE_RESTARTS) times. Infinite recursion?")
endif
