EXTRA_DIST = stardict_youdaodict.v

if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_youdaodict.v"
endif

noinst_HEADERS = stardict_youdaodict.h

stardict_youdaodict_LTLIBRARIES = stardict_youdaodict.la

stardict_youdaodictdir = $(libdir)/stardict/plugins

stardict_youdaodict_la_SOURCES = stardict_youdaodict.cpp

stardict_youdaodict_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) 
