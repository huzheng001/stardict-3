#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([StarDict tools], 3.0.3, [http://www.stardict.org])
AC_CONFIG_SRCDIR([src/stardict-editor.cpp])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([dist-bzip2])
AC_CONFIG_HEADERS([config.h])
AH_BOTTOM([#include <../lib/config-custom.h>])

AC_CONFIG_MACRO_DIR([m4])
LT_INIT

AC_DEFUN([AC_FIND_FILE],
[
	$3=NO
	for x in $2
	do
		for y in $1
		do
			if test -r "$x/$y"
			then
				$3=$x
				break 2
			fi
		done
	done
]
)

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])

# mysql.h
MYSQLFOUND=NO
AC_CHECK_HEADER([mysql.h],
	[
		MYSQL_CFLAGS=
		MYSQLFOUND=YES
	],
	[], []
)
if test x$MYSQLFOUND = xNO; then
	AC_FIND_FILE([mysql.h],
		[/usr/local/mysql/include /usr/include/mysql /usr/local/include/mysql],
		[MYSQLINCDIR])
	if test x$MYSQLINCDIR != xNO; then
		AC_CHECK_HEADER([$MYSQLINCDIR/mysql.h],
			[
				MYSQL_CFLAGS="-I${MYSQLINCDIR}"
				MYSQLFOUND=YES
			],
			[], []
		)
	fi
fi
if test x$MYSQLFOUND = xNO; then
	AC_ERROR([mysql.h header not found])
fi
AC_SUBST(MYSQL_CFLAGS)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
DEP_MODULES="gtk+-2.0 >= 2.6 glib-2.0 >= 2.8 zlib gio-2.0"
PKG_CHECK_MODULES(STARDICT, $DEP_MODULES)

# mysqlclient
MYSQLFOUND=NO
AC_CHECK_LIB([mysqlclient], [mysql_real_connect],
	[
		MYSQL_LIBS="-lmysqlclient"
		MYSQL_LDFLAGS=
		MYSQLFOUND=YES
	],
	[]
)
if test x$MYSQLFOUND = xNO; then
	AC_FIND_FILE([libmysqlclient.a],
		[/usr/lib /usr/lib/mysql /usr/local/mysql/lib /usr/local/lib/mysql],
		[MYSQLLIBDIR])
	if test x$MYSQLLIBDIR != xNO; then
		MYSQL_LIBS="-lmysqlclient"
		MYSQL_LDFLAGS="-L${MYSQLLIBDIR}"
		MYSQLFOUND=YES
		AC_MSG_RESULT([libmysqlclient.a found at ${MYSQLLIBDIR}])
	fi
fi
if test x$MYSQLFOUND = xNO; then
	AC_ERROR([libmysqlclient.a not found])
fi
AC_SUBST(MYSQL_LIBS)
AC_SUBST(MYSQL_LDFLAGS)

AC_ARG_ENABLE([deprecations],
  AS_HELP_STRING([--disable-deprecations],[Disable deprecated gtk functions (default: enabled)]),
  [enable_deprecations=$enableval],
  [enable_deprecations=yes])

if test "x$enable_deprecations" = "xno" ; then
  STARDICT_CFLAGS="$STARDICT_CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED"
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
PKG_CHECK_MODULES([LIBXML], [libxml-2.0 >= 2.5])

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
