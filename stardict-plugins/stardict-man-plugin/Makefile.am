EXTRA_DIST = stardict_man.v


LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_man.v"

noinst_HEADERS = stardict_man.h

stardict_man_LTLIBRARIES = stardict_man.la

stardict_mandir = $(libdir)/stardict/plugins

stardict_man_la_SOURCES = stardict_man.cpp

stardict_man_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

INCLUDES = @STARDICT_CFLAGS@
