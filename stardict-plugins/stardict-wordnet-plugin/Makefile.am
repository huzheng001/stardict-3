EXTRA_DIST = stardict_wordnet.v stardict_wordnet.dev


LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_wordnet.v"

noinst_HEADERS = stardict_wordnet.h

stardict_wordnet_LTLIBRARIES = stardict_wordnet.la

stardict_wordnetdir = $(libdir)/stardict/plugins

stardict_wordnet_la_SOURCES = stardict_wordnet.cpp

stardict_wordnet_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

INCLUDES = @STARDICT_CFLAGS@
