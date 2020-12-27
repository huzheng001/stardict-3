EXTRA_DIST = stardict_wordnet.v	\
		stardict_wordnet_parsedata.v


if DARWIN_SUPPORT
WORDNET_LD_VERSION_SCRIPT_OPTION=
else
WORDNET_LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_wordnet.v"
endif

stardict_wordnet_LTLIBRARIES = stardict_wordnet.la

stardict_wordnetdir = $(libdir)/stardict/plugins

stardict_wordnet_la_SOURCES = stardict_wordnet.cpp stardict_wordnet.h court_widget.cpp court_widget.h \
				geom.h newton.cpp newton_env.cpp newton_env.h newton.h partic.cpp partic.h scene.cpp scene.h \
				spring.cpp spring.h tenis.h utils.h vector_t.cpp \
				vector_t.h

stardict_wordnet_la_LDFLAGS = 	-avoid-version \
					-module \
					$(WORDNET_LD_VERSION_SCRIPT_OPTION)


if DARWIN_SUPPORT
WORDNET_PARSEDATA_LD_VERSION_SCRIPT_OPTION=
else
WORDNET_PARSEDATA_LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/stardict_wordnet_parsedata.v"
endif

stardict_wordnet_parsedata_LTLIBRARIES = stardict_wordnet_parsedata.la

stardict_wordnet_parsedatadir = $(libdir)/stardict/plugins

stardict_wordnet_parsedata_la_SOURCES = stardict_wordnet_parsedata.cpp \
											stardict_wordnet_parsedata.h

stardict_wordnet_parsedata_la_LDFLAGS = 	-avoid-version \
					-module \
					$(WORDNET_PARSEDATA_LD_VERSION_SCRIPT_OPTION)

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_srcdir) 
