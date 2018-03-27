AUTOMAKE_OPTIONS = foreign
AM_MAKEFLAGS = --no-print-directory

GIT_SHA1 ?= `git --work-tree=$(top_srcdir) --git-dir=$(top_srcdir)/.git describe --always --long --dirty || echo unknown`

bin_PROGRAMS = pdbg

ACLOCAL_AMFLAGS = -Im4
AM_CFLAGS = -I$(top_srcdir)/ccan/array_size -Wall -Werror -O2

if TARGET_ARM
DT_ARM = p8-fsi.dts p8-i2c.dts \
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

BUILT_SOURCES = $(DT_headers)

pdbg_SOURCES = \
	src/main.c src/cfam.c src/scom.c src/reg.c src/mem.c src/thread.c \
	src/ring.c src/htm.c src/progress.c src/options_@ARCH@.c

pdbg_LDADD = $(DT_objects) libpdbg.la libfdt.la \
	-L.libs

pdbg_LDFLAGS = -Wl,--whole-archive,-lpdbg,--no-whole-archive
pdbg_CFLAGS = -I$(top_srcdir)/libpdbg -Wall -Werror -DGIT_SHA1=\"${GIT_SHA1}\" \
	      $(ARCH_FLAGS)

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
	libpdbg/htm.c \
	libpdbg/debug.c

M4_V = $(M4_V_$(V))
M4_V_ = $(M4_V_$(AM_DEFAULT_VERBOSITY))
M4_V_0 = @echo "  M4      " $@;

DTC_V = $(DTC_V_$(V))
DTC_V_ = $(DTC_V_$(AM_DEFAULT_VERBOSITY))
DTC_V_0	= @echo "  DTC     " $@;

GEN_V = $(GEN_V_$(V))
GEN_V_ = $(GEN_V_$(AM_DEFAULT_VERBOSITY))
GEN_V_0 = @echo "  GEN     " $@;

%.dts: %.dts.m4
	$(M4_V)$(M4) -I$(dir $<) $< > $@

%.dtsi: %.dtsi.m4
	$(M4_V)$(M4) -I$(dir $<) $< > $@

p9w-fsi.dtb: p9w-fsi.dts p9-fsi.dtsi
p9r-fsi.dtb: p9r-fsi.dts p9-fsi.dtsi
p9z-fsi.dtb: p9z-fsi.dts p9-fsi.dtsi

%.dtb: %.dts
	$(DTC_V)$(DTC) -q -i$(dir $@) -I dts $< -O dtb > $@

%.dt.h: %.dtb
	$(GEN_V)$(srcdir)/generate_dt_header.sh $< > $@

%.dtb.o: %.dtb
	$(AM_V_CC)$(CC) -c $(srcdir)/template.S -DSYMBOL_PREFIX=$(shell echo $@ | tr '.-' '_') -DFILENAME=\"$<\" -o $@

MOSTLYCLEANFILES = *.dtb *.dt.h
