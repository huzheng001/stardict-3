EXTRA_DIST = stardict_qqwry.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_qqwry.v"
endif

noinst_HEADERS = stardict_qqwry.h

stardict_qqwry_LTLIBRARIES = stardict_qqwry.la

stardict_qqwrydir = $(libdir)/stardict/plugins

stardict_qqwry_la_SOURCES = stardict_qqwry.cpp

stardict_qqwry_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) 
