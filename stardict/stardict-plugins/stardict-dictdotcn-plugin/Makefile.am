EXTRA_DIST = stardict_dictdotcn.v

if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_dictdotcn.v"
endif

noinst_HEADERS = stardict_dictdotcn.h

stardict_dictdotcn_LTLIBRARIES = stardict_dictdotcn.la

stardict_dictdotcndir = $(libdir)/stardict/plugins

stardict_dictdotcn_la_SOURCES = stardict_dictdotcn.cpp

stardict_dictdotcn_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) 
