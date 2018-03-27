AC_INIT([pdbg], [1.0])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_LIBTOOL

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

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_LANG(C)

case "$host" in
	arm*-*-*)       ARCH="arm" ;;
	powerpc*-*-*)   ARCH="ppc" ;;
	*)              ARCH="def" ;;
esac
AC_SUBST([ARCH])

AC_OUTPUT
