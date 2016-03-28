EXTRA_DIST = stardict_info.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_info.v"
endif

noinst_HEADERS = stardict_info.h

stardict_info_LTLIBRARIES = stardict_info.la

stardict_infodir = $(libdir)/stardict/plugins

stardict_info_la_SOURCES = stardict_info.cpp

stardict_info_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) 
