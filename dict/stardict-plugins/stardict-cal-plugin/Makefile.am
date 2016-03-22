EXTRA_DIST = stardict_cal.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_cal.v"
endif

noinst_HEADERS = stardict_cal.h

stardict_cal_LTLIBRARIES = stardict_cal.la

stardict_caldir = $(libdir)/stardict/plugins

stardict_cal_la_SOURCES = stardict_cal.cpp

stardict_cal_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) 
