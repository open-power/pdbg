GIT_SHA1 = `git --work-tree=$(top_srcdir) --git-dir=$(top_srcdir)/.git describe --always --long --dirty || echo unknown`

bin_PROGRAMS = pdbg
lib_LIBRARIES = libpdbg.a

AM_CFLAGS = -I$(top_srcdir)/ccan/array_size -Wall

pdbg_SOURCES = \
	src/main.c
pdbg_LDADD = libpdbg.a
pdbg_CFLAGS = -I$(top_srcdir)/libpdbg -Wall -DGIT_SHA1=\"${GIT_SHA1}\"

libpdbg_a_SOURCES = \
	libpdbg/target.c \
	libpdbg/cfam.c \
	libpdbg/adu.c \
	libpdbg/bmcfsi.c \
	libpdbg/kernel.c \
	libpdbg/i2c.c \
	libpdbg/chip.c
