AC_INIT([pdbg],[3.3])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])
AC_SUBST(SONAME_CURRENT, 5)
AC_SUBST(SONAME_REVISION, 0)
AC_SUBST(SONAME_AGE, 0)

AC_PROG_CC
AC_PROG_CXX
AM_PROG_AS
LT_INIT

AC_PATH_PROG([M4], [m4])
if test x"$ac_cv_path_M4" = "x" ; then
	AC_MSG_ERROR([Command 'm4' not found.])
fi
AC_SUBST([M4])

AC_PATH_PROG([DTC], [dtc])
if test x"$ac_cv_path_DTC" = x ; then
	AC_MSG_ERROR([Command 'dtc' not found.])
fi
AC_SUBST([DTC])

AC_PATH_PROG([RAGEL], [ragel])
AC_SUBST([RAGEL])
AM_CONDITIONAL([HAVE_RAGEL], [test x"$ac_cv_path_RAGEL" != "x"])

AC_CHECK_LIB([fdt], [fdt_check_header])
AM_CONDITIONAL([BUILD_LIBFDT], [test x"$ac_cv_lib_fdt_fdt_check_header" != "xyes"])
if test x"$ac_cv_lib_fdt_fdt_check_header" != "xyes" ; then
	AC_CONFIG_SUBDIRS([libfdt])
else
	AC_SUBST([LIBFDT], ["-lfdt"])
fi

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([libpdbg/pdbg.pc])
AC_LANG(C)

case "$host" in
	arm*-*-*)       ARCH="arm" ;;
	powerpc*-*-*)   ARCH="ppc" ;;
	*)              ARCH="def" ;;
esac
AC_SUBST([ARCH])

AM_CONDITIONAL([TARGET_ARM], [test x"$ARCH" = "xarm"])
AM_CONDITIONAL([TARGET_PPC], [test x"$ARCH" = "xppc"])

AC_ARG_ENABLE(gdbserver,
AS_HELP_STRING([--disable-gdbserver], [disables building the gdbserver]),
want_gdbserver=false,
want_gdbserver=true)
AM_CONDITIONAL([GDBSERVER], [test x$want_gdbserver = xtrue])

AC_OUTPUT
