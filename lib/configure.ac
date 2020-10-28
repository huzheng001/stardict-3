#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([StarDict lib], 3.0.7, [http://stardict-4.sourceforge.net])
AC_CONFIG_SRCDIR([src/libcommon.cpp])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([tar-ustar dist-xz])
AC_CONFIG_HEADERS([config.h])
AH_BOTTOM([#include <config-custom.h>])

AC_CONFIG_MACRO_DIR([m4])
LT_INIT

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
DEP_MODULES="gtk+-3.0 glib-2.0 >= 2.8 gmodule-2.0 zlib libxml-2.0 >= 2.5"
PKG_CHECK_MODULES(STARDICT, $DEP_MODULES)

AC_ARG_ENABLE([deprecations],
  AS_HELP_STRING([--disable-deprecations],[Disable deprecated gtk functions (default: enabled)]),
  [enable_deprecations=$enableval],
  [enable_deprecations=yes])

if test "x$enable_deprecations" = "xno" ; then
  STARDICT_CFLAGS="$STARDICT_CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED"
  STARDICT_CFLAGS="$STARDICT_CFLAGS -DG_DISABLE_SINGLE_INCLUDES -DATK_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"
fi

STARDICT_CFLAGS="-Wall $STARDICT_CFLAGS"

AC_CONFIG_FILES([
Makefile
src/Makefile
])
AC_OUTPUT
