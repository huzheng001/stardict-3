EXTRA_DIST = stardict_gucharmap.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_gucharmap.v"
endif

noinst_HEADERS = stardict_gucharmap.h

stardict_gucharmap_LTLIBRARIES = stardict_gucharmap.la

stardict_gucharmapdir = $(libdir)/stardict/plugins

stardict_gucharmap_la_SOURCES = stardict_gucharmap.cpp

stardict_gucharmap_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

stardict_gucharmap_la_LIBADD  = $(GUCHARMAP_LIBS)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) -I$(top_builddir)/src $(GUCHARMAP_CFLAGS)
