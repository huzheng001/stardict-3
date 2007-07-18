EXTRA_DIST = stardict_html_parsedata.v stardict_html_parsedata.dev


LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_html_parsedata.v"

noinst_HEADERS = stardict_html_parsedata.h

stardict_html_parsedata_LTLIBRARIES = stardict_html_parsedata.la

stardict_html_parsedatadir = $(libdir)/stardict/plugins

stardict_html_parsedata_la_SOURCES = stardict_html_parsedata.cpp

stardict_html_parsedata_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

INCLUDES = @STARDICT_CFLAGS@ -I$(top_builddir)/src
