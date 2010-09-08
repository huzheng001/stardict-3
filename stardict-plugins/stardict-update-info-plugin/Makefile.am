EXTRA_DIST = stardict_update_info.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_update_info.v"
endif

noinst_HEADERS = stardict_update_info.h

stardict_update_info_LTLIBRARIES = stardict_update_info.la

stardict_update_infodir = $(libdir)/stardict/plugins

stardict_update_info_la_SOURCES = stardict_update_info.cpp

stardict_update_info_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) -I$(top_builddir)/src
