#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([StarDict], 3.0.7, [http://stardict-4.sourceforge.net])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([1.9 tar-ustar dist-xz])

AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
# Checks for libraries.
# Checks for header files.
# Checks for typedefs, structures, and compiler characteristics.
# Checks for library functions.

AC_ARG_ENABLE([dict],
  AS_HELP_STRING([--disable-dict],[Disable StarDict dictionary (default: enabled)]),
  [enable_dict=$enableval],
  [enable_dict=yes])

AC_ARG_ENABLE([tools],
  AS_HELP_STRING([--disable-tools],[Disable StarDict tools (default: enabled)]),
  [enable_tools=$enableval],
  [enable_tools=yes])

AM_CONDITIONAL([BUILD_DICT], [test "x$enable_dict" = xyes])
AM_CONDITIONAL([BUILD_TOOLS], [test "x$enable_tools" = xyes])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS([lib])
if test "x$enable_dict" = "xyes" ; then
  AC_CONFIG_SUBDIRS([dict])
fi
if test "x$enable_tools" = "xyes" ; then
  AC_CONFIG_SUBDIRS([tools])
fi
AC_OUTPUT


echo "

Type \"make\" to compile stardict.

"
