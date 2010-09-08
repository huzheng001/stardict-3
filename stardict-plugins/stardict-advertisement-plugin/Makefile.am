SUBDIRS = data

EXTRA_DIST = advertisement.v

if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/advertisement.v"
endif

noinst_HEADERS = advertisement.h

advertisement_LTLIBRARIES = advertisement.la

advertisementdir = $(libdir)/stardict/plugins

advertisement_la_SOURCES = advertisement.cpp

advertisement_la_LDFLAGS = 	-avoid-version \
				-module \
				$(LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) -I$(top_builddir)/src
