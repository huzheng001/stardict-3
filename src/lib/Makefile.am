EXTRA_DIST = dict_client.cpp dict_client.hpp

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
	sockets.cpp sockets.hpp \
	http_client.cpp http_client.h	\
	httpmanager.cpp httpmanager.h	\
	plugin.cpp plugin.h \
	virtualdictplugin.cpp virtualdictplugin.h \
	netdictplugin.cpp netdictplugin.h \
	specialdictplugin.cpp specialdictplugin.h \
	netdictcache.cpp netdictcache.h	\
	ttsplugin.cpp ttsplugin.h	\
	parsedata_plugin.cpp parsedata_plugin.h	\
	pluginmanager.cpp pluginmanager.h	\
	dictmask.h	\
	getuint32.h	\
	stardict_libconfig.h

if USE_SYSTEM_SIGCPP
LOCAL_SIGCPP_INCLUDE =
else
LOCAL_SIGCPP_INCLUDE = -I$(top_builddir)/src -I$(top_builddir)/src/sigc++config
endif

INCLUDES = @STARDICT_CFLAGS@ $(LOCAL_SIGCPP_INCLUDE)
