EXTRA_DIST = stardict_update_info.v stardict_update_info.dev


LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_update_info.v"

noinst_HEADERS = stardict_update_info.h

stardict_update_info_LTLIBRARIES = stardict_update_info.la

stardict_update_infodir = $(libdir)/stardict/plugins

stardict_update_info_la_SOURCES = stardict_update_info.cpp

stardict_update_info_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

INCLUDES = @STARDICT_CFLAGS@ -I$(top_builddir)/src
