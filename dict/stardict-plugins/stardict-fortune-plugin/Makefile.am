EXTRA_DIST = stardict_fortune.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_fortune.v"
endif

noinst_HEADERS = stardict_fortune.h

stardict_fortune_LTLIBRARIES = stardict_fortune.la

stardict_fortunedir = $(libdir)/stardict/plugins

stardict_fortune_la_SOURCES = stardict_fortune.cpp

stardict_fortune_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) 
