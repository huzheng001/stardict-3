#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([StarDict tools], 3.0.7, [http://stardict-4.sourceforge.net])
AC_CONFIG_SRCDIR([src/stardict-editor.cpp])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([tar-ustar dist-xz])
AC_CONFIG_HEADERS([config.h])
AH_BOTTOM([#include <../lib/config-custom.h>])

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
DEP_MODULES="gtk+-3.0 glib-2.0 >= 2.8 zlib gio-2.0"
PKG_CHECK_MODULES(STARDICT, $DEP_MODULES)

# mysqlclient
AC_ARG_WITH(mysql-config, 
[  --with-mysql-config=PATH   The path to mysql-config if not in $PATH], [
  MYSQLCONFIG=$withval
], [
  MYSQLCONFIG=mysql-config-was-not-found-by-configure
])

if test "x$MYSQLCONFIG" = "xmysql-config-was-not-found-by-configure"; then
	AC_PATH_PROG( MYSQLCONFIG, mysql_config, mysql_config-was-not-found-by-configure,
              $PATH:/usr/bin:/usr/local/bin)
fi

AC_SUBST(MYSQLCONFIG)
if test "x$MYSQLCONFIG" = "xmysql_config-was-not-found-by-configure"; then
  AC_MSG_ERROR([mysql_config wasn't found please install libmysqlclient-dev http://www.mysql.com ])
else
  MYSQL_CFLAGS=`$MYSQLCONFIG --cflags`
  MYSQL_INCLUDES=`$MYSQLCONFIG --include`
  MYSQL_LIBS=`$MYSQLCONFIG --libs_r`
fi
AC_SUBST(MYSQL_CFLAGS)
AC_SUBST(MYSQL_INCLUDES)
AC_SUBST(MYSQL_LIBS)

AC_MSG_CHECKING(for MySQL CFLAGS)
AC_MSG_RESULT($MYSQL_CFLAGS)
AC_MSG_CHECKING(for MySQL includes)
AC_MSG_RESULT($MYSQL_INCLUDES)
AC_MSG_CHECKING(for MySQL libraries)
AC_MSG_RESULT($MYSQL_LIBS)


AC_ARG_ENABLE([deprecations],
  AS_HELP_STRING([--disable-deprecations],[Disable deprecated gtk functions (default: enabled)]),
  [enable_deprecations=$enableval],
  [enable_deprecations=yes])

if test "x$enable_deprecations" = "xno" ; then
  STARDICT_CFLAGS="$STARDICT_CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED"
  STARDICT_CFLAGS="$STARDICT_CFLAGS -DG_DISABLE_SINGLE_INCLUDES -DATK_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"
fi

STARDICT_CFLAGS="-Wall $STARDICT_CFLAGS"

LFS_CFLAGS=`getconf LFS_CFLAGS`
LFS_LDFLAGS=`getconf LFS_LDFLAGS`
LFS_LIBS=`getconf LFS_LIBS`
AC_SUBST(LFS_CFLAGS)
AC_SUBST(LFS_LDFLAGS)
AC_SUBST(LFS_LIBS)

dnl For src/dsl2dict.c
PKG_CHECK_MODULES(DSL2DICT, libpcre)
AC_CHECK_FUNCS([getline mempcpy])

dnl Many programs use iconv function. Determine whether it is available and the
dnl type of the second argument: whether it is char ** or const char **.
dnl The AM_ICONV macro is a part of gettext package.
dnl In case AM_ICONV is not able to find the iconv library, consider setting
dnl LDFLAGS environment variable to specify library path. For example, 
dnl export LDFLAGS="-L/usr/local/lib".
AM_ICONV

if test "x$am_cv_func_iconv" != xyes; then
AC_ERROR([iconv not found])
fi


dnl for src/stardict_text2bin
PKG_CHECK_MODULES([LIBXML], [libxml-2.0 >= 2.5], have_libxml=yes, have_libxml=no)
if test "x$have_libxml" = "xyes" ; then
	LIBXML_CFLAGS="${LIBXML_CFLAGS}"
	LIBXML_LIBS="${LIBXML_LIBS}"
else
	AC_MSG_ERROR([LIBXML-2.0 library not found or too old.])
fi

AC_SUBST(LIBXML_CFLAGS)
AC_SUBST(LIBXML_LIBS)


dnl for src/stardict-editor
PKG_CHECK_MODULES([EXPAT], [expat], have_expat=yes, have_expat=no)
if test "x$have_expat" = "xyes" ; then
	EXPAT_CFLAGS="${EXPAT_CFLAGS}"
	EXPAT_LIBS="${EXPAT_LIBS}"
else
	AC_MSG_ERROR([EXPAT library not found or too old.])
fi

AC_SUBST(EXPAT_CFLAGS)
AC_SUBST(EXPAT_LIBS)


dnl common lib
dnl path relative to $(top_srcdir)
COMMONLIB_INCLUDE_DIR=../lib/src
AC_SUBST(COMMONLIB_INCLUDE_DIR)
dnl path relative to $(top_builddir)
COMMONLIB_LIBRARY=../lib/src/libcommon.la
AC_SUBST(COMMONLIB_LIBRARY)

AC_CONFIG_FILES([
Makefile
src/Makefile
])
AC_OUTPUT
