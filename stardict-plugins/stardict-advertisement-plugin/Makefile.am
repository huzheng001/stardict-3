SUBDIRS = data

EXTRA_DIST = advertisement.v advertisement.dev

LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=advertisement.v"

noinst_HEADERS = advertisement.h

advertisement_LTLIBRARIES = advertisement.la

advertisementdir = $(libdir)/stardict/plugins

advertisement_la_SOURCES = advertisement.cpp

advertisement_la_LDFLAGS = 	-avoid-version \
				-module \
				$(LD_VERSION_SCRIPT_OPTION)

INCLUDES = @STARDICT_CFLAGS@ -I$(top_builddir)/src
