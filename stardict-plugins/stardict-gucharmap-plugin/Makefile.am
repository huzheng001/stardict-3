EXTRA_DIST = stardict_gucharmap.v


LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_gucharmap.v"

noinst_HEADERS = stardict_gucharmap.h

stardict_gucharmap_LTLIBRARIES = stardict_gucharmap.la

stardict_gucharmapdir = $(libdir)/stardict/plugins

stardict_gucharmap_la_SOURCES = stardict_gucharmap.cpp

stardict_gucharmap_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

stardict_gucharmap_la_LIBADD  = $(GUCHARMAP_LIBS)

INCLUDES = @STARDICT_CFLAGS@ -I$(top_builddir)/src $(GUCHARMAP_CFLAGS)
