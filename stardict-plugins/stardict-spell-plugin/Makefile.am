EXTRA_DIST = stardict_spell.v


LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_spell.v"

noinst_HEADERS = stardict_spell.h

stardict_spell_LTLIBRARIES = stardict_spell.la

stardict_spelldir = $(libdir)/stardict/plugins

stardict_spell_la_SOURCES = stardict_spell.cpp

stardict_spell_la_LDFLAGS = 	-avoid-version \
				-module \
				$(LD_VERSION_SCRIPT_OPTION)

stardict_spell_la_LIBADD  = $(ENCHANT_LIBS)

INCLUDES = @STARDICT_CFLAGS@ -I$(top_builddir)/src $(ENCHANT_CFLAGS)
