bin_PROGRAMS = pdbg
lib_LIBRARIES = libpdbg.a

AM_CFLAGS = -I$(top_srcdir)/ccan/array_size

pdbg_SOURCES = \
	src/main.c
pdbg_LDADD = libpdbg.a
pdbg_CFLAGS = -I$(top_srcdir)/libpdbg

libpdbg_a_SOURCES = \
	libpdbg/adu.c \
	libpdbg/bmcfsi.c \
	libpdbg/i2c.c \
	libpdbg/ram.c \
	libpdbg/scom.c
