dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.52)

AC_INIT([StarDict], 3.0.7, [http://stardict-4.sourceforge.net])
AC_CONFIG_SRCDIR(src/stardict.cpp)
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([1.9 tar-ustar dist-xz])

AC_CONFIG_HEADERS([config.h])
AH_BOTTOM([#include <../lib/config-custom.h>])
AM_DISABLE_STATIC

AC_PATH_PROG(GCONFTOOL, gconftool-2)
IT_PROG_INTLTOOL([0.22])
PKG_PROG_PKG_CONFIG

AC_CONFIG_MACRO_DIR(m4)
LT_INIT
GNOME_DOC_INIT

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CXX
AC_LANG([C++])
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_FUNC_MMAP
AC_CHECK_FUNC(gethostbyname_r, AC_DEFINE([HAVE_GETHOSTBYNAME_R], [], [Have gethostbyname_r function.]))
AC_CHECK_FUNC(memrchr, AC_DEFINE([HAVE_MEMRCHR], [], [Have memrchr function.]))


dnl ================================================================
dnl Gettext stuff.
dnl ================================================================
GETTEXT_PACKAGE=stardict
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
AM_GLIB_GNU_GETTEXT


dnl ================================================================
dnl spell plugin checks: enchant.
dnl ================================================================

ENCHANT1_REQUIRED=1.2.0
ENCHANT2_REQUIRED=2.2.3
AC_ARG_ENABLE([spell],
	AS_HELP_STRING([--disable-spell],[Disable spell plugin (default: enabled)]),
	[enable_enchant=$enableval],
	[enable_enchant=yes])

if test "x$enable_enchant" = "xyes" ; then
	PKG_CHECK_MODULES([ENCHANT2], enchant-2 >= $ENCHANT2_REQUIRED, \
			have_enchant2=yes, have_enchant2=no)
	PKG_CHECK_MODULES([ENCHANT1], enchant >= $ENCHANT1_REQUIRED, \
			have_enchant1=yes, have_enchant1=no)
	if test "x$have_enchant2" = "xyes"; then
		if test "x$have_enchant1" = "xyes"; then
			AC_MSG_ERROR([You have both Enchant 1.x and Enchant 2.x installed. This is not supported.])
		else
			ENCHANT_CFLAGS="${ENCHANT2_CFLAGS}"
			ENCHANT_LIBS="${ENCHANT2_LIBS}"
			SPELL_PLUGIN_DIR="stardict-spell-plugin"
		fi
	else
		if test "x$have_enchant1" = "xyes"; then
			ENCHANT_CFLAGS="${ENCHANT1_CFLAGS}"
			ENCHANT_LIBS="${ENCHANT1_LIBS}"
			SPELL_PLUGIN_DIR="stardict-spell-plugin"
		else
			AC_MSG_ERROR([Enchant library not found or too old. Use --disable-spell to build without spell plugin.])
		fi
	fi
else
	ENCHANT_CFLAGS=
	ENCHANT_LIBS=
	SPELL_PLUGIN_DIR=
fi

AC_SUBST(ENCHANT_CFLAGS)
AC_SUBST(ENCHANT_LIBS)
AC_SUBST(SPELL_PLUGIN_DIR)

dnl ================================================================
dnl gucharmap plugin checks.
dnl ================================================================

AC_ARG_ENABLE([gucharmap],
	AS_HELP_STRING([--disable-gucharmap],[Disable gucharmap plugin (default: enabled)]),
	[enable_gucharmap=$enableval],
	[enable_gucharmap=yes])

if test "x$enable_gucharmap" = "xyes" ; then
	PKG_CHECK_MODULES(GUCHARMAP, [gucharmap-2.90], have_gucharmap=yes, have_gucharmap=no)
	if test "x$have_gucharmap" = "xyes"; then
		GUCHARMAP_CFLAGS="${GUCHARMAP_CFLAGS}"
		GUCHARMAP_LIBS="${GUCHARMAP_LIBS}"
		GUCHARMAP_PLUGIN_DIR="stardict-gucharmap-plugin"
	else
		AC_MSG_ERROR([Gucharmap library not found or too old. Use --disable-gucharmap to build without gucharmap plugin.])
	fi
else
	GUCHARMAP_CFLAGS=
	GUCHARMAP_LIBS=
	GUCHARMAP_PLUGIN_DIR=
fi

AC_SUBST(GUCHARMAP_CFLAGS)
AC_SUBST(GUCHARMAP_LIBS)
AC_SUBST(GUCHARMAP_PLUGIN_DIR)

dnl ================================================================
dnl festival tts plugin checks.
dnl ================================================================

AC_ARG_ENABLE([festival],
	AS_HELP_STRING([--disable-festival],[Disable festival tts plugin (default: enabled)]),
	[enable_festival=$enableval],
	[enable_festival=yes])

if test "x$enable_festival" = "xyes" ; then
	dnl festival.h includes EST.h, it must be in include path
	CPPFLAGS="${CPPFLAGS} -I/usr/include/speech_tools"
	AC_CHECK_HEADERS([festival/festival.h],
		[
			FESTIVAL_LIBS="-lFestival -lestools -lestbase -leststring"
			FESTIVAL_CFLAGS="-I/usr/include/speech_tools -ffriend-injection -Wno-deprecated"
			FESTIVAL_PLUGIN_DIR="stardict-festival-tts-plugin"
		],
		[
			AC_MSG_ERROR([Festival library not found. Use --disable-festival to build without festival plugin.])
		],
		[]
	)
else
	FESTIVAL_CFLAGS=
	FESTIVAL_LIBS=
	FESTIVAL_PLUGIN_DIR=
fi

AC_SUBST(FESTIVAL_CFLAGS)
AC_SUBST(FESTIVAL_LIBS)
AC_SUBST(FESTIVAL_PLUGIN_DIR)

dnl ================================================================
dnl Flite tts plugin checks.
dnl ================================================================

AC_ARG_ENABLE([flite],
	AS_HELP_STRING([--disable-flite],[Disable flite tts plugin (default: enabled)]),
	[enable_flite=$enableval],
	[enable_flite=yes])

if test "x$enable_flite" = "xyes" ; then
	dnl flite.h, it must be in include path
	AC_CHECK_HEADERS([flite/flite.h],
		[
			FLITE_LIBS="-lflite_cmu_us_kal -lflite_cmu_us_kal16 -lflite_cmu_us_rms -lflite_cmu_time_awb -lflite_cmu_us_slt -lflite_cmu_us_awb -lflite_usenglish -lflite_cmulex -lflite -lm"
			FLITE_CFLAGS=""
			FLITE_PLUGIN_DIR="stardict-flite-tts-plugin"
		],
		[
			AC_MSG_ERROR([Flite library not found. Use --disable-flite to build without flite plugin.])
		],
		[]
	)
else
	FLITE_CFLAGS=
	FLITE_LIBS=
	FLITE_PLUGIN_DIR=
fi

AC_SUBST(FLITE_CFLAGS)
AC_SUBST(FLITE_LIBS)
AC_SUBST(FLITE_PLUGIN_DIR)

dnl ================================================================
dnl espeak tts plugin checks.
dnl ================================================================

AC_ARG_ENABLE([espeak],
	AS_HELP_STRING([--disable-espeak],[Disable espeak tts plugin (default: enabled)]),
	[enable_espeak=$enableval],
	[enable_espeak=yes])

if test "x$enable_espeak" = "xyes" ; then
	AC_CHECK_HEADERS([espeak/speak_lib.h],
		[
			ESPEAK_LIBS="-lespeak"
			ESPEAK_CFLAGS=
			ESPEAK_PLUGIN_DIR="stardict-espeak-tts-plugin"
		],
		[
			AC_MSG_ERROR([Espeak library not found. Use --disable-espeak to build without espeak plugin.])
		],
		[]
	)
else
	ESPEAK_CFLAGS=
	ESPEAK_LIBS=
	ESPEAK_PLUGIN_DIR=
fi

AC_SUBST(ESPEAK_CFLAGS)
AC_SUBST(ESPEAK_LIBS)
AC_SUBST(ESPEAK_PLUGIN_DIR)

dnl ================================================================
dnl espeak-ng tts plugin checks.
dnl ================================================================

AC_ARG_ENABLE([espeak_ng],
	AS_HELP_STRING([--disable-espeak-ng],[Disable espeak-ng tts plugin (default: enabled)]),
	[enable_espeak_ng=$enableval],
	[enable_espeak_ng=yes])

if test "x$enable_espeak_ng" = "xyes" ; then
	AC_CHECK_HEADERS([espeak-ng/speak_lib.h],
		[
			ESPEAK_NG_LIBS="-lespeak-ng"
			ESPEAK_NG_CFLAGS=
			ESPEAK_NG_PLUGIN_DIR="stardict-espeak-ng-tts-plugin"
		],
		[
			AC_MSG_ERROR([Espeak-ng library not found. Use --disable-espeak-ng to build without espeak-ng plugin.])
		],
		[]
	)
else
	ESPEAK_NG_CFLAGS=
	ESPEAK_NG_LIBS=
	ESPEAK_NG_PLUGIN_DIR=
fi

AC_SUBST(ESPEAK_NG_CFLAGS)
AC_SUBST(ESPEAK_NG_LIBS)
AC_SUBST(ESPEAK_NG_PLUGIN_DIR)


dnl ================================================================
dnl other plugin checks.
dnl ================================================================

AC_ARG_ENABLE([dictdotcn],
	AS_HELP_STRING([--disable-dictdotcn],[Disable dictdotcn netdict plugin (default: enabled)]),
	[enable_dictdotcn=$enableval],
	[enable_dictdotcn=yes])
if test "x$enable_dictdotcn" = "xyes" ; then
	DICTDOTCN_PLUGIN_DIR="stardict-dictdotcn-plugin"
else
	DICTDOTCN_PLUGIN_DIR=
fi
AC_SUBST(DICTDOTCN_PLUGIN_DIR)


AC_ARG_ENABLE([youdaodict],
	AS_HELP_STRING([--disable-youdaodict],[Disable youdaodict netdict plugin (default: enabled)]),
	[enable_youdaodict=$enableval],
	[enable_youdaodict=yes])
if test "x$enable_youdaodict" = "xyes" ; then
	YOUDAODICT_PLUGIN_DIR="stardict-youdaodict-plugin"
else
	YOUDAODICT_PLUGIN_DIR=
fi
AC_SUBST(YOUDAODICT_PLUGIN_DIR)


AC_ARG_ENABLE([customdict],
	AS_HELP_STRING([--disable-customdict],[Disable customdict virtual dict plugin (default: enabled)]),
	[enable_customdict=$enableval],
	[enable_customdict=yes])
if test "x$enable_customdict" = "xyes" ; then
	CUSTOMDICT_PLUGIN_DIR="stardict-customdict-plugin"
else
	CUSTOMDICT_PLUGIN_DIR=
fi
AC_SUBST(CUSTOMDICT_PLUGIN_DIR)

AC_ARG_ENABLE([man],
	AS_HELP_STRING([--disable-man],[Disable man virtual dict plugin (default: enabled)]),
	[enable_man=$enableval],
	[enable_man=yes])
if test "x$enable_man" = "xyes" ; then
	MAN_PLUGIN_DIR="stardict-man-plugin"
else
	MAN_PLUGIN_DIR=
fi
AC_SUBST(MAN_PLUGIN_DIR)

AC_ARG_ENABLE([info],
	AS_HELP_STRING([--disable-info],[Disable info virtual dict plugin (default: enabled)]),
	[enable_info=$enableval],
	[enable_info=yes])
if test "x$enable_info" = "xyes" ; then
	INFO_PLUGIN_DIR="stardict-info-plugin"
else
	INFO_PLUGIN_DIR=
fi
AC_SUBST(INFO_PLUGIN_DIR)

AC_ARG_ENABLE([fortune],
	AS_HELP_STRING([--disable-fortune],[Disable fortune virtual dict plugin (default: enabled)]),
	[enable_fortune=$enableval],
	[enable_fortune=yes])
if test "x$enable_fortune" = "xyes" ; then
	FORTUNE_PLUGIN_DIR="stardict-fortune-plugin"
else
	FORTUNE_PLUGIN_DIR=
fi
AC_SUBST(FORTUNE_PLUGIN_DIR)

AC_ARG_ENABLE([cal],
	AS_HELP_STRING([--disable-cal],[Disable cal virtual dict plugin (default: enabled)]),
	[enable_cal=$enableval],
	[enable_cal=yes])
if test "x$enable_cal" = "xyes" ; then
	CAL_PLUGIN_DIR="stardict-cal-plugin"
else
	CAL_PLUGIN_DIR=
fi
AC_SUBST(CAL_PLUGIN_DIR)

AC_ARG_ENABLE([qqwry],
	AS_HELP_STRING([--disable-qqwry],[Disable qqwry virtual dict plugin (default: enabled)]),
	[enable_qqwry=$enableval],
	[enable_qqwry=yes])
if test "x$enable_qqwry" = "xyes" ; then
	QQWRY_PLUGIN_DIR="stardict-qqwry-plugin"
else
	QQWRY_PLUGIN_DIR=
fi
AC_SUBST(QQWRY_PLUGIN_DIR)

AC_ARG_ENABLE([updateinfo],
	AS_HELP_STRING([--disable-updateinfo],[Disable update-info plugin (default: enabled)]),
	[enable_updateinfo=$enableval],
	[enable_updateinfo=yes])
if test "x$enable_updateinfo" = "xyes" ; then
	UPDATE_INFO_PLUGIN_DIR="stardict-update-info-plugin"
else
	UPDATE_INFO_PLUGIN_DIR=
fi
AC_SUBST(UPDATE_INFO_PLUGIN_DIR)

AC_ARG_ENABLE([htmlparse],
	AS_HELP_STRING([--disable-htmlparse],[Disable html parse plugin (default: enabled)]),
	[enable_htmlparse=$enableval],
	[enable_htmlparse=yes])
if test "x$enable_htmlparse" = "xyes" ; then
	HTML_PARSEDATA_PLUGIN_DIR="stardict-html-parsedata-plugin"
else
	HTML_PARSEDATA_PLUGIN_DIR=
fi
AC_SUBST(HTML_PARSEDATA_PLUGIN_DIR)

AC_ARG_ENABLE([xdxfparse],
	AS_HELP_STRING([--disable-xdxfparse],[Disable xdxf parse plugin (default: enabled)]),
	[enable_xdxfparse=$enableval],
	[enable_xdxfparse=yes])
if test "x$enable_xdxfparse" = "xyes" ; then
	XDXF_PARSEDATA_PLUGIN_DIR="stardict-xdxf-parsedata-plugin"
else
	XDXF_PARSEDATA_PLUGIN_DIR=
fi
AC_SUBST(XDXF_PARSEDATA_PLUGIN_DIR)

AC_ARG_ENABLE([wordnet],
	AS_HELP_STRING([--disable-wordnet],[Disable wordnet plugin (default: enabled)]),
	[enable_wordnet=$enableval],
	[enable_wordnet=yes])
if test "x$enable_wordnet" = "xyes" ; then
	WORDNET_PLUGIN_DIR="stardict-wordnet-plugin"
else
	WORDNET_PLUGIN_DIR=
fi
AC_SUBST(WORDNET_PLUGIN_DIR)

AC_ARG_ENABLE([powerwordparse],
	AS_HELP_STRING([--disable-powerwordparse],[Disable powerword parse plugin (default: enabled)]),
	[enable_powerwordparse=$enableval],
	[enable_powerwordparse=yes])
if test "x$enable_powerwordparse" = "xyes" ; then
	POWERWORD_PARSEDATA_PLUGIN_DIR="stardict-powerword-parsedata-plugin"
else
	POWERWORD_PARSEDATA_PLUGIN_DIR=
fi
AC_SUBST(POWERWORD_PARSEDATA_PLUGIN_DIR)

AC_ARG_ENABLE([wikiparse],
	AS_HELP_STRING([--disable-wikiparse],[Disable wiki parse plugin (default: enabled)]),
	[enable_wikiparse=$enableval],
	[enable_wikiparse=yes])
if test "x$enable_wikiparse" = "xyes" ; then
	WIKI_PARSEDATA_PLUGIN_DIR="stardict-wiki-parsedata-plugin"
else
	WIKI_PARSEDATA_PLUGIN_DIR=
fi
AC_SUBST(WIKI_PARSEDATA_PLUGIN_DIR)

dnl ================================================================
dnl main program checks.
dnl ================================================================

AC_ARG_ENABLE(gnome-support, AC_HELP_STRING([--disable-gnome-support], [Disable gnome support]))
AC_ARG_ENABLE(gpe-support, AC_HELP_STRING([--enable-gpe-support], [Enable gpe support]))
AC_ARG_ENABLE(maemo-support, AC_HELP_STRING([--enable-maemo-support], [Enable maemo support]))
AC_ARG_ENABLE(darwin-support, AC_HELP_STRING([--enable-darwin-support], [Enable darwin support]))
AM_CONDITIONAL(GNOME_SUPPORT, test "x${enable_gpe_support}" != "xyes" && test "x${enable_maemo_support}" != "xyes" && test "x${enable_darwin_support}" != "xyes" && test "x${enable_gnome_support}" != "xno")
AM_CONDITIONAL(GPE_SUPPORT, test "x${enable_gpe_support}" = "xyes")
AM_CONDITIONAL(MAEMO_SUPPORT, test "x${enable_maemo_support}" = "xyes")
AM_CONDITIONAL(DARWIN_SUPPORT, test "x${enable_darwin_support}" = "xyes")

if test "x${enable_gpe_support}" = "xyes" ; then
  DEP_MODULES="gtk+-3.0 glib-2.0 >= 2.32 gmodule-2.0 gthread-2.0 libcanberra libcanberra-gtk3 libgpewidget >= 0.109"
elif test "x${enable_maemo_support}" = "xyes" ; then
  DEP_MODULES="gtk+-3.0 glib-2.0 >= 2.32 gmodule-2.0 gthread-2.0 libcanberra libcanberra-gtk3 gconf-2.0 >= 2.6 hildon-libs >= 0.12"
elif test "x${enable_darwin_support}" = "xyes" ; then
  DEP_MODULES="gtk+-3.0 glib-2.0 >= 2.32 gmodule-2.0 gthread-2.0 libcanberra libcanberra-gtk3"
elif test "x${enable_gnome_support}" = "xno" ; then
  DEP_MODULES="gtk+-3.0 glib-2.0 >= 2.32 gmodule-2.0 gthread-2.0 libcanberra libcanberra-gtk3"
else
  DEP_MODULES="gtk+-3.0 glib-2.0 >= 2.32 gmodule-2.0 gthread-2.0 libcanberra libcanberra-gtk3 libgnome-2.0 libbonobo-2.0 gconf-2.0"
fi

dnl
dnl Start of pkg-config checks
dnl
PKG_CHECK_MODULES(STARDICT, $DEP_MODULES)

if test "x${enable_gpe_support}" = "xyes" ; then
        echo "Enable gpe suppport";
	AC_DEFINE([CONFIG_GPE],[], [Compile with gpe support])
	AM_CONDITIONAL(GCONF_SCHEMAS_INSTALL, test "x${enable_gpe_support}" != "xyes")
elif test "x${enable_maemo_support}" = "xyes" ; then
        echo "Enable maemo suppport";
	AC_DEFINE([CONFIG_MAEMO],[], [Compile with maemo support])
	AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
	if test x"$GCONFTOOL" = xno; then
        	AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
	fi
	AM_CONDITIONAL(GCONF_SCHEMAS_INSTALL, test "x${enable_maemo_support}" != "xno")
	AM_GCONF_SOURCE_2
elif test "x${enable_darwin_support}" = "xyes" ; then
        echo "Enable darwin suppport";
	AC_DEFINE([CONFIG_DARWIN],[], [Compile with darwin support])
	AM_CONDITIONAL(GCONF_SCHEMAS_INSTALL, test "x${enable_darwin_support}" != "xyes")
elif test "x${enable_gnome_support}" = "xno" ; then
	echo "Disable gnome support";
	AC_DEFINE([CONFIG_GTK],[], [Compile without gnome support])
	AM_CONDITIONAL(GCONF_SCHEMAS_INSTALL, test "x${enable_gnome_support}" != "xno")
else
	echo "Enable gnome suppport";
	AC_DEFINE([CONFIG_GNOME],[], [Compile with gnome support])
	AM_CONDITIONAL(GCONF_SCHEMAS_INSTALL, test "x${enable_gnome_support}" != "xno")

dnl *********************************************
dnl for anything which calls gconftool-2 correctly
dnl *********************************************

	AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
	if test x"$GCONFTOOL" = xno; then
        	AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
	fi
	AM_GCONF_SOURCE_2

dnl ==========================================================================
dnl stuff for the CORBA interface

	ORBIT_IDL="`$PKG_CONFIG --variable=orbit_idl ORBit-2.0`"
	AC_SUBST(ORBIT_IDL)

	LIBBONOBO_IDL="`$PKG_CONFIG --variable=idldir libbonobo-2.0`"
	AC_SUBST(LIBBONOBO_IDL)

	BONOBO_ACTIVATION_IDL="`$PKG_CONFIG --variable=idldir bonobo-activation-2.0`"
	AC_SUBST(BONOBO_ACTIVATION_IDL)
fi


AC_ARG_ENABLE([deprecations],
	AS_HELP_STRING([--disable-deprecations],[Disable deprecated gtk functions (default: enabled)]),
	[enable_deprecations=$enableval],
	[enable_deprecations=yes])

if test "x$enable_deprecations" = "xno" ; then
	STARDICT_CFLAGS="$STARDICT_CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED"
	STARDICT_CFLAGS="$STARDICT_CFLAGS -DG_DISABLE_SINGLE_INCLUDES -DATK_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"
fi

STARDICT_CFLAGS="-Wall $STARDICT_CFLAGS"


dnl
dnl Start of zlib checks
dnl
AC_CHECK_LIB(z,zlibVersion,STARDICT_LIBS="$STARDICT_LIBS -lz",[AC_MSG_ERROR([zlib not found, or gcc g++ not installed])])
# For the first AC_CHECK_LIB, if failed, it may because of compiler didn't installed. So add this warning for the first AC_CHECK_LIB macro.


dnl ================================================================
dnl libsigc++20 checks.
dnl ================================================================

PKG_CHECK_MODULES(SIGCPP, "sigc++-2.0", [use_system_sigcpp=yes], [use_system_sigcpp=no])
AM_CONDITIONAL(USE_SYSTEM_SIGCPP, test "x${use_system_sigcpp}" = "xyes")
if test "x${use_system_sigcpp}" = "xyes" ; then
echo "Use system libsigc++20";
STARDICT_CFLAGS="$SIGCPP_CFLAGS $STARDICT_CFLAGS"
STARDICT_LIBS="$SIGCPP_LIBS $STARDICT_LIBS"
else
echo "Use local libsigc++20";
SIGC_CXX_GCC_TEMPLATE_SPECIALIZATION_OPERATOR_OVERLOAD()
SIGC_CXX_MSVC_TEMPLATE_SPECIALIZATION_OPERATOR_OVERLOAD()
SIGC_CXX_SELF_REFERENCE_IN_MEMBER_INITIALIZATION()
SIGC_CXX_HAS_NAMESPACE_STD()
SIGC_CXX_HAS_SUN_REVERSE_ITERATOR()

if test "X$config_error" = "Xyes" ; then
  AC_ERROR(
[One or more of the required compiler features is missing.
If you believe this is in error, please consult the config.log file
for further details.
])
fi

fi

dnl ================================================================
dnl scrollkeeper checks.
dnl It is used in help subdir
dnl ================================================================
if test "x$enable_scrollkeeper" = "xyes" ; then
	AC_CHECK_PROG([SCROLLKEEPER], [scrollkeeper-config], [yes])
	if test x"$SCROLLKEEPER" != xyes; then
		AC_MSG_ERROR([scrollkeeper-config executable is not found in your path - should be installed with scrollkeeper])
	fi
fi

dnl ==========================================================================

if test "x${enable_darwin_support}" != "xyes" ; then
# Checks for header files. need by Solaris.
AC_PATH_XTRA
if test -n "$X_CFLAGS"; then
CPPFLAGS="$CPPFLAGS $X_CFLAGS"
fi
if test -n "$X_LIBS"; then
LDFLAGS="$LDFLAGS $X_LIBS"
fi
if test -n "$X_PRE_LIBS"; then
LDFLAGS="$LDFLAGS $X_PRE_LIBS"
fi
if test -n "$X_EXTRA_LIBS"; then
LDFLAGS="$LDFLAGS $X_EXTRA_LIBS"
fi

# Checks for libraries.
AC_CHECK_LIB([X11], [main], , [AC_MSG_ERROR([X11 lib not found])])
fi

AC_SUBST(STARDICT_LIBS)
AC_SUBST(STARDICT_CFLAGS)


dnl common lib
dnl path relative to $(top_srcdir)
COMMONLIB_INCLUDE_DIR=../lib/src
AC_SUBST(COMMONLIB_INCLUDE_DIR)
dnl path relative to $(top_builddir)
COMMONLIB_LIBRARY=../lib/src/libcommon.la
AC_SUBST(COMMONLIB_LIBRARY)

dnl ==========================================================================

AC_CONFIG_FILES([
Makefile
stardict.spec
stardict-withoutgnome.spec
data/Makefile
doc/Makefile
doc/wiki/Makefile
debian-upstream/Makefile
mac_dist/Makefile
mac_dist/zh_CN.lproj/Makefile
msvc_2008/Makefile
po/Makefile.in
m4/Makefile
src/Makefile
src/sigc++/Makefile
src/sigc++config/Makefile
src/lib/Makefile
src/pixmaps/Makefile
src/sounds/Makefile
src/dic/Makefile
src/dic/stardict-dict/Makefile
src/dic/stardict-dict/res/Makefile
src/skins/Makefile
src/treedict/Makefile
src/win32/Makefile
src/win32/acrobat/Makefile
src/win32/acrobat/sources/Makefile
src/win32/acrobat/win32/Makefile
src/win32/nsis/Makefile
src/win32/PortableApps.com/Makefile
stardict-plugins/Makefile
stardict-plugins/stardict-customdict-plugin/Makefile
stardict-plugins/stardict-customdict-plugin/data/Makefile
stardict-plugins/stardict-man-plugin/Makefile
stardict-plugins/stardict-info-plugin/Makefile
stardict-plugins/stardict-fortune-plugin/Makefile
stardict-plugins/stardict-cal-plugin/Makefile
stardict-plugins/stardict-qqwry-plugin/Makefile
stardict-plugins/stardict-dictdotcn-plugin/Makefile
stardict-plugins/stardict-youdaodict-plugin/Makefile
stardict-plugins/stardict-update-info-plugin/Makefile
stardict-plugins/stardict-spell-plugin/Makefile
stardict-plugins/stardict-gucharmap-plugin/Makefile
stardict-plugins/stardict-festival-tts-plugin/Makefile
stardict-plugins/stardict-flite-tts-plugin/Makefile
stardict-plugins/stardict-espeak-tts-plugin/Makefile
stardict-plugins/stardict-espeak-ng-tts-plugin/Makefile
stardict-plugins/stardict-wiki-parsedata-plugin/Makefile
stardict-plugins/stardict-powerword-parsedata-plugin/Makefile
stardict-plugins/stardict-xdxf-parsedata-plugin/Makefile
stardict-plugins/stardict-wordnet-plugin/Makefile
stardict-plugins/stardict-html-parsedata-plugin/Makefile
stardict-plugins/stardict-sapi-tts-plugin/Makefile
tests/Makefile
pixmaps/Makefile
help/Makefile
])
AC_OUTPUT
