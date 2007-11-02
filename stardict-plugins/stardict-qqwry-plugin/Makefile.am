EXTRA_DIST = stardict_qqwry.v stardict_qqwry.dev


LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_qqwry.v"

noinst_HEADERS = stardict_qqwry.h

stardict_qqwry_LTLIBRARIES = stardict_qqwry.la

stardict_qqwrydir = $(libdir)/stardict/plugins

stardict_qqwry_la_SOURCES = stardict_qqwry.cpp

stardict_qqwry_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

INCLUDES = @STARDICT_CFLAGS@
