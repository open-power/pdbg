AUTOMAKE_OPTIONS = foreign
AM_MAKEFLAGS = --no-print-directory

GIT_SHA1 ?= `git --work-tree=$(top_srcdir) --git-dir=$(top_srcdir)/.git describe --always --long --dirty || echo unknown`

include libfdt/Makefile.libfdt

bin_PROGRAMS = pdbg

ACLOCAL_AMFLAGS = -Im4
AM_CFLAGS = -I$(top_srcdir)/ccan/array_size -Wall -Werror -O2

pdbg_SOURCES = \
	src/main.c src/cfam.c src/scom.c src/reg.c src/mem.c src/thread.c \
	src/ring.c src/htm.c src/options_@ARCH@.c

pdbg_LDADD = fake.dtb.o p8-fsi.dtb.o p8-i2c.dtb.o p9w-fsi.dtb.o	p8-host.dtb.o \
	p9z-fsi.dtb.o p9r-fsi.dtb.o p9-kernel.dtb.o libpdbg.la libfdt.la \
	p9-host.dtb.o \
	-L.libs

pdbg_LDFLAGS = -Wl,--whole-archive,-lpdbg,--no-whole-archive
pdbg_CFLAGS = -I$(top_srcdir)/libpdbg -Wall -Werror -DGIT_SHA1=\"${GIT_SHA1}\"

lib_LTLIBRARIES = libpdbg.la libfdt.la

libfdt_la_CFLAGS = -I$(top_srcdir)/libfdt -DHAVE_LITTLE_ENDIAN
libpdbg_la_CFLAGS = -I$(top_srcdir)/libfdt -DHAVE_LITTLE_ENDIAN -Wall -Werror

libfdt_la_SOURCES = \
	libfdt/fdt.c \
	libfdt/fdt_ro.c \
	libfdt/fdt_wip.c \
	libfdt/fdt_sw.c \
	libfdt/fdt_rw.c \
	libfdt/fdt_strerror.c \
	libfdt/fdt_empty_tree.c \
	libfdt/fdt_addresses.c \
	libfdt/fdt_overlay.c

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
	libpdbg/progress.c \
	libpdbg/htm.c

M4_V = $(M4_V_$(V))
M4_V_ = $(M4_V_$(AM_DEFAULT_VERBOSITY))
M4_V_0 = @echo "  M4      " $@;

DTC_V = $(DTC_V_$(V))
DTC_V_ = $(DTC_V_$(AM_DEFAULT_VERBOSITY))
DTC_V_0	= @echo "  DTC     " $@;

%.dts: %.dts.m4
	$(M4_V)m4 -I$(dir $<) $< > $@

%.dtsi: %.dtsi.m4
	$(M4_V)m4 -I$(dir $<) $< > $@

p9w-fsi.dtb: p9w-fsi.dts p9-fsi.dtsi
p9r-fsi.dtb: p9r-fsi.dts p9-fsi.dtsi
p9z-fsi.dtb: p9z-fsi.dts p9-fsi.dtsi

%.dtb: %.dts
	$(DTC_V)dtc -q -i$(dir $@) -I dts $< -O dtb > $@

%.dtb.o: %.dtb
	$(AM_V_CC)$(CC) -c $(srcdir)/template.S -DSYMBOL_PREFIX=$(shell echo $@ | tr '.-' '_') -DFILENAME=\"$<\" -o $@

MOSTLYCLEANFILES = *.dtb
