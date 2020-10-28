SUBDIRS = data

EXTRA_DIST = customdict.v

if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/customdict.v"
endif

noinst_HEADERS = customdict.h

customdict_LTLIBRARIES = customdict.la

customdictdir = $(libdir)/stardict/plugins

customdict_la_SOURCES = customdict.cpp

customdict_la_LDFLAGS = 	-avoid-version \
				-module \
				$(LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) -I$(top_builddir)/src
