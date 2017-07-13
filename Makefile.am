GIT_SHA1 = `git --work-tree=$(top_srcdir) --git-dir=$(top_srcdir)/.git describe --always --long --dirty || echo unknown`

include libfdt/Makefile.libfdt

bin_PROGRAMS = pdbg

ACLOCAL_AMFLAGS = -Im4
AM_CFLAGS = -I$(top_srcdir)/ccan/array_size -Wall

pdbg_SOURCES = \
	src/main.c
pdbg_LDADD = fake.dtb.o p8-fsi.dtb.o p8-i2c.dtb.o p9w-fsi.dtb.o	\
	p9z-fsi.dtb.o p9r-fsi.dtb.o p9-kernel.dtb.o

pdbg_LDFLAGS = -L.libs -Wl,--whole-archive,-lpdbg,--no-whole-archive -lfdt
pdbg_CFLAGS = -I$(top_srcdir)/libpdbg -Wall -DGIT_SHA1=\"${GIT_SHA1}\" -g

lib_LTLIBRARIES = libpdbg.la libfdt.la

libfdt_la_CFLAGS = -I$(top_srcdir)/libfdt -DHAVE_LITTLE_ENDIAN
libpdbg_la_CFLAGS = -I$(top_srcdir)/libfdt -DHAVE_LITTLE_ENDIAN

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
	libpdbg/target.c

%.dts: %.dts.m4
	m4 -Isrc $< > $@

%.dtsi: %.dtsi.m4
	m4 -Isrc $< > $@

p9w-fsi.dtb.o: p9w-fsi.dts p9-fsi.dtsi
%.dtb.o: %.dts
	dtc -I dts $< -O dtb > $@.tmp

# We need to align the output as some processor/kernel
# combinations can't deal with the alignment errors when
# unflattening the device-tree
	dd if=$@.tmp of=$@ ibs=16 conv=sync
	rm $@.tmp
	objcopy -I binary -O elf32-littlearm -B arm $@ $@
#	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $@ $@
