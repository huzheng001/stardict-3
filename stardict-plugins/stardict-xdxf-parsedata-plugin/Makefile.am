EXTRA_DIST = stardict_xdxf_parsedata.v stardict_xdxf_parsedata.dev


LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_xdxf_parsedata.v"

noinst_HEADERS = stardict_xdxf_parsedata.h

stardict_xdxf_parsedata_LTLIBRARIES = stardict_xdxf_parsedata.la

stardict_xdxf_parsedatadir = $(libdir)/stardict/plugins

stardict_xdxf_parsedata_la_SOURCES = stardict_xdxf_parsedata.cpp

stardict_xdxf_parsedata_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

INCLUDES = @STARDICT_CFLAGS@ -I$(top_builddir)/src
