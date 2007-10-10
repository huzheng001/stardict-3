EXTRA_DIST = stardict_wordnet.v stardict_wordnet.dev	\
		stardict_wordnet_parsedata.v stardict_wordnet_parsedata.dev


WORDNET_LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_wordnet.v"

stardict_wordnet_LTLIBRARIES = stardict_wordnet.la

stardict_wordnetdir = $(libdir)/stardict/plugins

stardict_wordnet_la_SOURCES = stardict_wordnet.cpp stardict_wordnet.h court_widget.cpp court_widget.h \
				geom.hpp newton.cpp newton_env.cpp newton_env.hpp newton.hpp partic.cpp partic.hpp scene.cpp scene.hpp spring.cpp spring.hpp tenis.hpp utils.hpp vector_t.cpp vector_t.hpp

stardict_wordnet_la_LDFLAGS = 	-avoid-version \
					-module \
					$(WORDNET_LD_VERSION_SCRIPT_OPTION)


WORDNET_PARSEDATA_LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_wordnet_parsedata.v"

stardict_wordnet_parsedata_LTLIBRARIES = stardict_wordnet_parsedata.la

stardict_wordnet_parsedatadir = $(libdir)/stardict/plugins

stardict_wordnet_parsedata_la_SOURCES = stardict_wordnet_parsedata.cpp stardict_wordnet_parsedata.h

stardict_wordnet_parsedata_la_LDFLAGS = 	-avoid-version \
					-module \
					$(WORDNET_PARSEDATA_LD_VERSION_SCRIPT_OPTION)

INCLUDES = @STARDICT_CFLAGS@
