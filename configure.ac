AC_INIT([pdbg], [0.3])
AM_INIT_AUTOMAKE([subdir-objects])

AC_PROG_CC
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_LANG(C)
AC_SUBST([ARCH_FF])
AC_COMPILE_IFELSE(
	[AC_LANG_SOURCE([[]])],
	ARCH_FF=$(${OBJDUMP} -f conftest.$OBJEXT |
	sed -ne "s/.*file format //p" -e "s/.*architecture: \(.*\),.*/\1/p" |
	tr "\n" " " |
	sed -e "s/ / -B /"),
	AC_MSG_FAILURE([Unable to determine architecture output file format]))
AC_OUTPUT
