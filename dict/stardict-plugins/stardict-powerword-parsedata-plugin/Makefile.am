EXTRA_DIST = stardict_powerword_parsedata.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_powerword_parsedata.v"
endif

noinst_HEADERS = stardict_powerword_parsedata.h

stardict_powerword_parsedata_LTLIBRARIES = stardict_powerword_parsedata.la

stardict_powerword_parsedatadir = $(libdir)/stardict/plugins

stardict_powerword_parsedata_la_SOURCES = stardict_powerword_parsedata.cpp

stardict_powerword_parsedata_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) -I$(top_builddir)/src
