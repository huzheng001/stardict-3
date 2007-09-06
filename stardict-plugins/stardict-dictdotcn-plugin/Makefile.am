EXTRA_DIST = stardict_dictdotcn.v dictdotcn.dev


LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_dictdotcn.v"

noinst_HEADERS = stardict_dictdotcn.h

stardict_dictdotcn_LTLIBRARIES = stardict_dictdotcn.la

stardict_dictdotcndir = $(libdir)/stardict/plugins

stardict_dictdotcn_la_SOURCES = stardict_dictdotcn.cpp

stardict_dictdotcn_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

INCLUDES = @STARDICT_CFLAGS@
