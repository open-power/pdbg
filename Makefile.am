bin_PROGRAMS = pdbg
lib_LIBRARIES = libpdbg.a

AM_CFLAGS = -Iccan/array_size

pdbg_SOURCES = \
	src/main.c
pdbg_LDADD = libpdbg.a
pdbg_CFLAGS = -Ilibpdbg

libpdbg_a_SOURCES = \
	libpdbg/adu.c \
	libpdbg/bmcfsi.c \
	libpdbg/ram.c \
	libpdbg/scom.c
