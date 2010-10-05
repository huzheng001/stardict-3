EXTRA_DIST = stardict_espeak.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_espeak.v"
endif

noinst_HEADERS = stardict_espeak.h

stardict_espeak_LTLIBRARIES = stardict_espeak.la

stardict_espeakdir = $(libdir)/stardict/plugins

stardict_espeak_la_SOURCES = stardict_espeak.cpp

stardict_espeak_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

stardict_espeak_la_LIBADD  = $(ESPEAK_LIBS)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) -I$(top_builddir)/src $(ESPEAK_CFLAGS)
