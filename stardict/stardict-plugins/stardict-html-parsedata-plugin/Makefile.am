EXTRA_DIST = stardict_html_parsedata.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_html_parsedata.v"
endif

noinst_HEADERS = stardict_html_parsedata.h

stardict_html_parsedata_LTLIBRARIES = stardict_html_parsedata.la

stardict_html_parsedatadir = $(libdir)/stardict/plugins

stardict_html_parsedata_la_SOURCES = stardict_html_parsedata.cpp

stardict_html_parsedata_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) -I$(top_builddir)/src
