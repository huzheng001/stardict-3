SUBDIRS = data

EXTRA_DIST = advertisement.v advertisement.dev

if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=advertisement.v"
endif

noinst_HEADERS = advertisement.h

advertisement_LTLIBRARIES = advertisement.la

advertisementdir = $(libdir)/stardict/plugins

advertisement_la_SOURCES = advertisement.cpp

advertisement_la_LDFLAGS = 	-avoid-version \
				-module \
				$(LD_VERSION_SCRIPT_OPTION)

INCLUDES = @STARDICT_CFLAGS@ -I$(top_builddir)/src
