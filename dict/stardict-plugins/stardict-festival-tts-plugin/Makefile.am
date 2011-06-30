EXTRA_DIST = stardict_festival.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_festival.v"
endif

noinst_HEADERS = stardict_festival.h

stardict_festival_LTLIBRARIES = stardict_festival.la

stardict_festivaldir = $(libdir)/stardict/plugins

stardict_festival_la_SOURCES = stardict_festival.cpp

stardict_festival_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

stardict_festival_la_LIBADD  = $(FESTIVAL_LIBS)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) -I$(top_builddir)/src $(FESTIVAL_CFLAGS)
