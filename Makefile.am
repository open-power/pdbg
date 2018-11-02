AUTOMAKE_OPTIONS = foreign
AM_MAKEFLAGS = --no-print-directory

GIT_SHA1 ?= `git --work-tree=$(top_srcdir) --git-dir=$(top_srcdir)/.git describe --always --long --dirty 2>/dev/null || echo unknown`

libpdbg_tests = libpdbg_target_test \
		libpdbg_probe_test1 \
		libpdbg_probe_test2 \
		libpdbg_probe_test3

bin_PROGRAMS = pdbg
check_PROGRAMS = $(libpdbg_tests) optcmd_test

PDBG_TESTS = \
	tests/test_selection.sh 	\
	tests/test_hw_bmc.sh

TESTS = $(libpdbg_tests) optcmd_test $(PDBG_TESTS)

test: $(libpdbg_tests)

TEST_EXTENSIONS = .sh
SH_LOG_DRIVER = $(SHELL) $(srcdir)/tests/run_test.sh

ACLOCAL_AMFLAGS = -Im4
AM_CFLAGS = -I$(top_srcdir)/ccan/array_size -Wall -Werror -O2

EXTRA_DIST = \
	fake.dts.m4 \
	p8-fsi.dts.m4 \
	p8-host.dts.m4 \
	p8-i2c.dts.m4 \
	p8-kernel.dts.m4 \
	p8-pib.dts.m4 \
	p9-fsi.dtsi.m4 \
	p9-host.dts.m4 \
	p9-kernel.dts.m4 \
	p9-pib.dts.m4 \
	p9r-fsi.dts.m4 \
	p9w-fsi.dts.m4 \
	p9z-fsi.dts.m4 \
	template.S \
	generate_dt_header.sh \
	tests/driver.sh \
	tests/run_test.sh \
	tests/test_driver.sh \
	$(PDBG_TESTS)

if TARGET_ARM
DT_ARM = p8-fsi.dts p8-i2c.dts p8-kernel.dts \
	 p9w-fsi.dts p9r-fsi.dts p9z-fsi.dts p9-kernel.dts
ARCH_FLAGS="-DTARGET_ARM=1"
endif

if TARGET_PPC
DT_PPC = p8-host.dts p9-host.dts
ARCH_FLAGS="-DTARGET_PPC=1"
endif

DT = fake.dts $(DT_ARM) $(DT_PPC)

DT_objects = $(DT:.dts=.dtb.o)
DT_headers = $(DT:.dts=.dt.h)

optcmd_test_SOURCES = src/optcmd.c src/parsers.c src/tests/optcmd_test.c
optcmd_test_CFLAGS = -Wall -g

if HAVE_RAGEL
extra_sources = src/gdb_parser.c
else
extra_sources = src/gdb_parser_precompile.c
endif

pdbg_SOURCES = \
	src/main.c \
	src/cfam.c \
	src/scom.c \
	src/reg.c \
	src/mem.c \
	src/thread.c \
	src/ring.c \
	src/htm.c \
	src/progress.c \
	src/parsers.c \
	src/optcmd.c \
	src/options_@ARCH@.c \
	src/pdbgproxy.c

pdbg_CFLAGS = -I$(top_srcdir)/libpdbg -Wall -Werror -DGIT_SHA1=\"${GIT_SHA1}\" \
	      $(ARCH_FLAGS)

if GDBSERVER
pdbg_SOURCES += \
	$(extra_sources)
else
pdbg_CFLAGS += -DDISABLE_GDBSERVER
endif

src/main.c: $(DT_headers)

src/pdbg-gdb_parser.$(OBJEXT): CFLAGS+=-Wno-unused-const-variable

pdbg_LDADD = $(DT_objects) libpdbg.la libccan.a \
	-L.libs -lrt

pdbg_LDFLAGS = -Wl,--whole-archive,-lpdbg,--no-whole-archive

lib_LTLIBRARIES = libpdbg.la libfdt.la

libfdt_la_CFLAGS = -I$(top_srcdir)/libfdt
libpdbg_la_CFLAGS = -I$(top_srcdir)/libfdt -Wall -Werror

libfdt_la_SOURCES = \
	libfdt/fdt.c \
	libfdt/fdt_ro.c \
	libfdt/fdt_wip.c \
	libfdt/fdt_sw.c \
	libfdt/fdt_rw.c \
	libfdt/fdt_strerror.c \
	libfdt/fdt_empty_tree.c \
	libfdt/fdt_addresses.c \
	libfdt/fdt_overlay.c \
	libfdt/fdt.h \
	libfdt/libfdt_env.h \
	libfdt/libfdt.h \
	libfdt/libfdt_internal.h

libpdbg_la_SOURCES = \
	libpdbg/libpdbg.c \
	libpdbg/host.c \
	libpdbg/kernel.c \
	libpdbg/fake.c \
	libpdbg/chip.c \
	libpdbg/p8chip.c \
	libpdbg/p9chip.c \
	libpdbg/bmcfsi.c \
	libpdbg/cfam.c \
	libpdbg/i2c.c \
	libpdbg/adu.c \
	libpdbg/device.c \
	libpdbg/target.c \
	libpdbg/htm.c \
	libpdbg/debug.c \
	libpdbg/backend.h \
	libpdbg/bitutils.h \
	libpdbg/compiler.h \
	libpdbg/debug.h \
	libpdbg/device.h \
	libpdbg/operations.h \
	libpdbg/libpdbg.h \
	libpdbg/target.h
libpdbg_la_LIBADD = libfdt.la

noinst_LIBRARIES = libccan.a

libccan_a_SOURCES = \
	ccan/array_size/array_size.h \
	ccan/build_assert/build_assert.h \
	ccan/check_type/check_type.h \
	ccan/container_of/container_of.h \
	ccan/cppmagic/cppmagic.h \
	ccan/list/list.c \
	ccan/list/list.h \
	ccan/short_types/short_types.h \
	ccan/str/str.c \
	ccan/str/str.h

libpdbg_test_cflags = -I$(top_srcdir)/libpdbg -I$(top_srcdir)/libfdt
libpdbg_test_ldflags = -Wl,--whole-archive,-lpdbg,--no-whole-archive
libpdbg_test_ldadd = -L.libs libpdbg.la

libpdbg_target_test_SOURCES = src/tests/libpdbg_target_test.c
libpdbg_target_test_CFLAGS = $(libpdbg_test_cflags)
libpdbg_target_test_LDFLAGS = $(libpdbg_test_ldflags)
libpdbg_target_test_LDADD = fake.dtb.o $(libpdbg_test_ldadd)

src/tests/libpdbg_target_test.c: fake.dt.h

libpdbg_probe_test1_SOURCES = src/tests/libpdbg_probe_test.c
libpdbg_probe_test1_CFLAGS = $(libpdbg_test_cflags) -DTEST_ID=1
libpdbg_probe_test1_LDFLAGS = $(libpdbg_test_ldflags)
libpdbg_probe_test1_LDADD = fake.dtb.o $(libpdbg_test_ldadd)

libpdbg_probe_test2_SOURCES = src/tests/libpdbg_probe_test.c
libpdbg_probe_test2_CFLAGS = $(libpdbg_test_cflags) -DTEST_ID=2
libpdbg_probe_test2_LDFLAGS = $(libpdbg_test_ldflags)
libpdbg_probe_test2_LDADD = fake.dtb.o $(libpdbg_test_ldadd)

libpdbg_probe_test3_SOURCES = src/tests/libpdbg_probe_test.c
libpdbg_probe_test3_CFLAGS = $(libpdbg_test_cflags) -DTEST_ID=3
libpdbg_probe_test3_LDFLAGS = $(libpdbg_test_ldflags)
libpdbg_probe_test3_LDADD = fake.dtb.o $(libpdbg_test_ldadd)

src/tests/libpdbg_probe_test.c: fake.dt.h

M4_V = $(M4_V_$(V))
M4_V_ = $(M4_V_$(AM_DEFAULT_VERBOSITY))
M4_V_0 = @echo "  M4      " $@;

DTC_V = $(DTC_V_$(V))
DTC_V_ = $(DTC_V_$(AM_DEFAULT_VERBOSITY))
DTC_V_0	= @echo "  DTC     " $@;

GEN_V = $(GEN_V_$(V))
GEN_V_ = $(GEN_V_$(AM_DEFAULT_VERBOSITY))
GEN_V_0 = @echo "  GEN     " $@;

RAGEL_V = $(RAGEL_V_$(V))
RAGEL_V_ = $(RAGEL_V_$(AM_DEFAULT_VERBOSITY))
RAGEL_V_0 = @echo "  RAGEL   " $@;

%.dts: %.dts.m4
	$(M4_V)$(M4) -I$(dir $<) $< | $(DTC) -I dts -O dts > $@

%.dtsi: %.dtsi.m4
	$(M4_V)$(M4) -I$(dir $<) $< > $@

p9-fsi.dtsi: p9-fsi.dtsi.m4 p9-pib.dts.m4
p9w-fsi.dts: p9w-fsi.dts.m4 p9-fsi.dtsi
p9r-fsi.dts: p9r-fsi.dts.m4 p9-fsi.dtsi
p9z-fsi.dts: p9z-fsi.dts.m4 p9-fsi.dtsi

%.dtb: %.dts
	$(DTC_V)$(DTC) -i$(dir $@) -I dts $< -O dtb > $@

%.dt.h: %.dtb
	$(GEN_V)$(srcdir)/generate_dt_header.sh $< > $@

%.dtb.o: %.dtb
	$(AM_V_CC)$(CC) -c $(srcdir)/template.S -DSYMBOL_PREFIX=$(shell echo $@ | tr '.-' '_') -DFILENAME=\"$<\" -o $@

%.c: %.rl
	$(RAGEL_V)$(RAGEL) -o $@ $<

MOSTLYCLEANFILES = *.dtb *.dts *.dt.h p9-fsi.dtsi src/gdb_parser.c
