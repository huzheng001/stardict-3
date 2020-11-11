EXTRA_DIST = stardict_espeak_ng.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_espeak_ng.v"
endif

noinst_HEADERS = stardict_espeak_ng.h

stardict_espeak_ng_LTLIBRARIES = stardict_espeak_ng.la

stardict_espeak_ngdir = $(libdir)/stardict/plugins

stardict_espeak_ng_la_SOURCES = stardict_espeak_ng.cpp

stardict_espeak_ng_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

stardict_espeak_ng_la_LIBADD  = $(ESPEAK_NG_LIBS)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) -I$(top_builddir)/src $(ESPEAK_NG_CFLAGS)
