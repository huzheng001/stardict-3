EXTRA_DIST = stardict_powerword_parsedata.version-script


LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_powerword_parsedata.version-script"

noinst_HEADERS = stardict_powerword_parsedata.h

stardict_powerword_parsedata_LTLIBRARIES = stardict_powerword_parsedata.la

stardict_powerword_parsedatadir = $(libdir)/stardict/plugins

stardict_powerword_parsedata_la_SOURCES = stardict_powerword_parsedata.cpp

stardict_powerword_parsedata_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

INCLUDES = @STARDICT_CFLAGS@ -I$(top_builddir)/src
