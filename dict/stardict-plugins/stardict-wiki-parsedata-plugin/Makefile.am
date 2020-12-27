EXTRA_DIST = stardict_wiki_parsedata.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_wiki_parsedata.v"
endif

noinst_HEADERS = stardict_wiki_parsedata.h

stardict_wiki_parsedata_LTLIBRARIES = stardict_wiki_parsedata.la

stardict_wiki_parsedatadir = $(libdir)/stardict/plugins

stardict_wiki_parsedata_la_SOURCES = stardict_wiki_parsedata.cpp	\
					global.cpp global.h	\
					stardict_wiki2xml.cpp stardict_wiki2xml.h	\
					TXML.cpp TXML.h	\
					WIKI2XML.cpp \
					WIKI2XML.h

stardict_wiki_parsedata_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) -I$(top_builddir)/src
