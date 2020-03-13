AUTOMAKE_OPTIONS = foreign
AM_MAKEFLAGS = --no-print-directory

if BUILD_LIBFDT
MAYBE_LIBFDT = libfdt
endif
SUBDIRS = $(MAYBE_LIBFDT)
DIST_SUBDIRS = libfdt

GIT_SHA1 ?= `git --work-tree=$(top_srcdir) --git-dir=$(top_srcdir)/.git describe --always --long --dirty 2>/dev/null || echo unknown`

libpdbg_tests = libpdbg_target_test \
		libpdbg_probe_test1 \
		libpdbg_probe_test2 \
		libpdbg_probe_test3

bin_PROGRAMS = pdbg
check_PROGRAMS = $(libpdbg_tests) libpdbg_dtree_test \
		libpdbg_p9_fapi_translation_test \
		optcmd_test hexdump_test cronus_proxy \
		libpdbg_prop_test

PDBG_TESTS = \
	tests/test_selection.sh 	\
	tests/test_selection2.sh 	\
	tests/test_hw_bmc.sh		\
	tests/test_hexdump.sh		\
	tests/test_tree.sh		\
	tests/test_tree2.sh		\
	tests/test_prop.sh		\
	tests/test_p9_fapi_translation.sh

TESTS = $(libpdbg_tests) optcmd_test $(PDBG_TESTS)

tests/test_tree2.sh: fake2.dtb
tests/test_prop.sh: fake.dtb
tests/test_p9_fapi_translation.sh: p9.dtb p9-kernel.dtb

test: $(libpdbg_tests)

TEST_EXTENSIONS = .sh
SH_LOG_DRIVER = $(SHELL) $(srcdir)/tests/run_test.sh

ACLOCAL_AMFLAGS = -Im4
AM_CFLAGS = -I$(top_srcdir)/ccan/array_size -Wall -Werror -O2

EXTRA_DIST = \
	fake.dts.m4 \
	fake2.dts.m4 \
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
	src/gdb_parser.rl \
	tests/driver.sh \
	tests/run_test.sh \
	tests/test_driver.sh \
	$(PDBG_TESTS)

if TARGET_ARM
ARCH_FLAGS="-DTARGET_ARM=1"
endif

if TARGET_PPC
ARCH_FLAGS="-DTARGET_PPC=1"
endif

DT = fake.dts fake2.dts p8-cronus.dts p9-cronus.dts \
     p8-fsi.dts p8-i2c.dts p8-kernel.dts \
     p9w-fsi.dts p9r-fsi.dts p9z-fsi.dts p9-kernel.dts \
     p8-host.dts p9-host.dts p8.dts

DT_sources = $(DT:.dts=.dtb.S) p9.dtb.S
DT_headers = $(DT:.dts=.dt.h) p9.dt.h

optcmd_test_SOURCES = src/optcmd.c src/parsers.c src/tests/optcmd_test.c
optcmd_test_CFLAGS = -Wall -g

hexdump_test_SOURCES = src/util.c src/tests/hexdump_test.c
hexdump_test_CFLAGS = -Wall -g

cronus_proxy_SOURCES = libcronus/proxy.c
cronus_proxy_CFLAGS = -Wall -g

pdbg_SOURCES = \
	src/cfam.c \
	src/htm.c \
	src/htm.h \
	src/istep.c \
	src/main.c \
	src/main.h \
	src/mem.c \
	src/optcmd.c \
	src/optcmd.h \
	src/options.h \
	src/parsers.c \
	src/parsers.h \
	src/path.c \
	src/path.h \
	src/pdbgproxy.c \
	src/pdbgproxy.h \
	src/progress.c \
	src/progress.h \
	src/reg.c \
	src/ring.c \
	src/scom.c \
	src/thread.c \
	src/util.c \
	src/util.h

pdbg_CFLAGS = -I$(top_srcdir)/libpdbg -Wall -Werror -DGIT_SHA1=\"${GIT_SHA1}\" \
	      $(ARCH_FLAGS)

if GDBSERVER
if HAVE_RAGEL
pdbg_SOURCES += src/gdb_parser.c
else
pdbg_SOURCES += src/gdb_parser_precompile.c
endif
else
pdbg_CFLAGS += -DDISABLE_GDBSERVER
endif

libpdbg/dtb.c: $(DT_headers)

src/pdbg-gdb_parser.$(OBJEXT): CFLAGS+=-Wno-unused-const-variable

pdbg_LDADD = libpdbg.la libccan.a \
	-L.libs -lrt

pdbg_LDFLAGS = -Wl,--whole-archive,-lpdbg,--no-whole-archive

lib_LTLIBRARIES = libpdbg.la
noinst_LTLIBRARIES = libcronus.la libsbefifo.la

libcronus_la_SOURCES = \
	libcronus/buffer.c \
	libcronus/buffer.h \
	libcronus/cfam.c \
	libcronus/connect.c \
	libcronus/instruction.h \
	libcronus/libcronus.h \
	libcronus/libcronus_private.h \
	libcronus/request.c \
	libcronus/sbefifo.c \
	libcronus/scom.c

libsbefifo_la_SOURCES = \
	libsbefifo/cmd_array.c \
	libsbefifo/cmd_control.c \
	libsbefifo/cmd_generic.c \
	libsbefifo/cmd_instruction.c \
	libsbefifo/cmd_memory.c \
	libsbefifo/cmd_mpipl.c \
	libsbefifo/cmd_register.c \
	libsbefifo/cmd_ring.c \
	libsbefifo/cmd_scom.c \
	libsbefifo/connect.c \
	libsbefifo/ffdc.c \
	libsbefifo/libsbefifo.h \
	libsbefifo/operation.c \
	libsbefifo/sbefifo_private.h

libpdbg_la_SOURCES = \
	$(DT_sources) \
	libpdbg/adu.c \
	libpdbg/bitutils.h \
	libpdbg/bmcfsi.c \
	libpdbg/cfam.c \
	libpdbg/chip.c \
	libpdbg/chip.h \
	libpdbg/compiler.h \
	libpdbg/cronus.c \
	libpdbg/debug.c \
	libpdbg/debug.h \
	libpdbg/device.c \
	libpdbg/dtb.c \
	libpdbg/fake.c \
	libpdbg/host.c \
	libpdbg/htm.c \
	libpdbg/hwunit.c \
	libpdbg/hwunit.h \
	libpdbg/i2c.c \
	libpdbg/kernel.c \
	libpdbg/libpdbg.c \
	libpdbg/libpdbg.h \
	libpdbg/operations.h \
	libpdbg/p8chip.c \
	libpdbg/p9chip.c \
	libpdbg/p9_fapi_targets.c \
	libpdbg/sbefifo.c \
	libpdbg/target.c \
	libpdbg/target.h

libpdbg_la_CFLAGS = -Wall -Werror
libpdbg_la_LIBADD = libcronus.la libsbefifo.la

if BUILD_LIBFDT
libpdbg_la_CFLAGS += -I$(top_srcdir)/libfdt
libpdbg_la_LIBADD += libfdt/libfdt.la
endif

include_HEADERS = libpdbg/libpdbg.h

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

if BUILD_LIBFDT
libpdbg_test_cflags += -I$(top_srcdir)/libfdt
endif

libpdbg_target_test_SOURCES = src/tests/libpdbg_target_test.c
libpdbg_target_test_CFLAGS = $(libpdbg_test_cflags)
libpdbg_target_test_LDFLAGS = $(libpdbg_test_ldflags)
libpdbg_target_test_LDADD = $(libpdbg_test_ldadd)

src/tests/libpdbg_target_test.c: fake.dt.h

libpdbg_probe_test1_SOURCES = src/tests/libpdbg_probe_test.c
libpdbg_probe_test1_CFLAGS = $(libpdbg_test_cflags) -DTEST_ID=1
libpdbg_probe_test1_LDFLAGS = $(libpdbg_test_ldflags)
libpdbg_probe_test1_LDADD = $(libpdbg_test_ldadd)

libpdbg_probe_test2_SOURCES = src/tests/libpdbg_probe_test.c
libpdbg_probe_test2_CFLAGS = $(libpdbg_test_cflags) -DTEST_ID=2
libpdbg_probe_test2_LDFLAGS = $(libpdbg_test_ldflags)
libpdbg_probe_test2_LDADD = $(libpdbg_test_ldadd)

libpdbg_probe_test3_SOURCES = src/tests/libpdbg_probe_test.c
libpdbg_probe_test3_CFLAGS = $(libpdbg_test_cflags) -DTEST_ID=3
libpdbg_probe_test3_LDFLAGS = $(libpdbg_test_ldflags)
libpdbg_probe_test3_LDADD = $(libpdbg_test_ldadd)

src/tests/libpdbg_probe_test.c: fake.dt.h

libpdbg_dtree_test_SOURCES = src/tests/libpdbg_dtree_test.c
libpdbg_dtree_test_CFLAGS = $(libpdbg_test_cflags)
libpdbg_dtree_test_LDFLAGS = $(libpdbg_test_ldflags)
libpdbg_dtree_test_LDADD = $(libpdbg_test_ldadd)

libpdbg_p9_fapi_translation_test_SOURCES = src/tests/libpdbg_p9_fapi_translation_test.C \
					   src/tests/p9_scominfo.C
libpdbg_p9_fapi_translation_test_CXXFLAGS = $(libpdbg_test_cflags)
libpdbg_p9_fapi_translation_test_LDFLAGS = $(libpdbg_test_ldflags)
libpdbg_p9_fapi_translation_test_LDADD = $(libpdbg_test_ldadd)

libpdbg_prop_test_SOURCES = src/tests/libpdbg_prop_test.c
libpdbg_prop_test_CFLAGS = $(libpdbg_test_cflags)
libpdbg_prop_test_LDFLAGS = $(libpdbg_test_ldflags)
libpdbg_prop_test_LDADD = $(libpdbg_test_ldadd)

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

%.dt.h: %.dts
	$(GEN_V)$(srcdir)/generate_dt_header.sh $< > $@

%.dtb.S: %.dtb
	$(GEN_V)$(CPP) $(srcdir)/template.S \
	-DSYMBOL_PREFIX=$(shell basename $< .S | tr '.-' '_')_o -DFILENAME=\"$<\" -o \
	$@

%.dtb.o: %.dtb.S
	$(AM_V_CC)$(CC) -o $@ $<

%.c: %.rl
	$(RAGEL_V)$(RAGEL) -o $@ $<

MOSTLYCLEANFILES = *.dtb.S *.dtb $(DT) *.dt.h p9-fsi.dtsi src/gdb_parser.c
