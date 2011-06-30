EXTRA_DIST = stardict_spell.v


if DARWIN_SUPPORT
LD_VERSION_SCRIPT_OPTION=
else
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_spell.v"
endif

noinst_HEADERS = stardict_spell.h

stardict_spell_LTLIBRARIES = stardict_spell.la

stardict_spelldir = $(libdir)/stardict/plugins

stardict_spell_la_SOURCES = stardict_spell.cpp

stardict_spell_la_LDFLAGS = 	-avoid-version \
				-module \
				$(LD_VERSION_SCRIPT_OPTION)

stardict_spell_la_LIBADD  = $(ENCHANT_LIBS)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) -I$(top_builddir)/src $(ENCHANT_CFLAGS)
