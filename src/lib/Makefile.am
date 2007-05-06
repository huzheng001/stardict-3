noinst_LIBRARIES = libstardict.a

libstardict_a_SOURCES = \
	lib.cpp lib.h	\
	dictziplib.cpp dictziplib.hpp	\
	distance.cpp distance.h	\
	mapfile.hpp file.hpp	\
	kmp.cpp kmp.h	\
	my_global.h m_ctype.h m_string.h	\
	ctype-mb.cpp ctype-utf8.cpp ctype-uca.cpp	\
	collation.cpp collation.h \
	data.hpp data.cpp \
	stddict.cpp stddict.hpp \
	storage.cpp storage.h	\
	treedict.cpp treedict.hpp	\
	common.cpp common.hpp \
	md5.c md5.h	\
	stardict_client.cpp stardict_client.hpp \
	dict_client.cpp dict_client.hpp \
	sockets.cpp sockets.hpp \
	plugin.cpp plugin.h \
	virtualdictplugin.cpp virtualdictplugin.h \
	pluginmanager.cpp pluginmanager.h	\
	dictmask.h

INCLUDES = @STARDICT_CFLAGS@ -I$(top_builddir)/src
