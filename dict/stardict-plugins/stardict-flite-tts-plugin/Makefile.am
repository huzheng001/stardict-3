EXTRA_DIST = stardict_flite.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_flite.v"
endif

noinst_HEADERS = stardict_flite.h

stardict_flite_LTLIBRARIES = stardict_flite.la

stardict_flitedir = $(libdir)/stardict/plugins

stardict_flite_la_SOURCES = stardict_flite.cpp

stardict_flite_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

stardict_flite_la_LIBADD  = $(FLITE_LIBS)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) -I$(top_builddir)/src $(FLITE_CFLAGS)
