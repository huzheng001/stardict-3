EXTRA_DIST = stardict_wordnet.v stardict_wordnet.dev


LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=stardict_wordnet.v"

noinst_HEADERS = stardict_wordnet.h court_widget.h

stardict_wordnet_LTLIBRARIES = stardict_wordnet.la

stardict_wordnetdir = $(libdir)/stardict/plugins

stardict_wordnet_la_SOURCES = stardict_wordnet.cpp court_widget.cpp \
				geom.hpp newton.cpp newton_env.cpp newton_env.hpp newton.hpp partic.cpp partic.hpp scene.cpp scene.hpp spring.cpp spring.hpp tenis.hpp utils.hpp vector_t.cpp vector_t.hpp

stardict_wordnet_la_LDFLAGS = 	-avoid-version \
					-module \
					$(LD_VERSION_SCRIPT_OPTION)

INCLUDES = @STARDICT_CFLAGS@
