EXTRA_DIST = dict_client.cpp dict_client.hpp

noinst_LIBRARIES = libstardict.a

libstardict_a_SOURCES = \
	dictziplib.cpp dictziplib.hpp	\
	edit-distance.cpp edit-distance.h	\
	mapfile.hpp file-utils.h	\
	kmp.cpp kmp.h	\
	m_ctype.h	\
	ctype-mb.cpp ctype-utf8.cpp ctype-uca.cpp	\
	collation.cpp collation.h \
	dictbase.h dictbase.cpp \
	stddict.cpp stddict.hpp \
	storage.cpp storage.h storage_impl.h	\
	treedict.cpp treedict.hpp	\
	ifo_file.cpp ifo_file.hpp \
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
	xml_str.cpp xml_str.h	\
	utils.cpp utils.h	\
	compositelookup.cpp compositelookup.h \
	stardict_libconfig.h stardict-sigc++.h \
	iappdirs.cpp iappdirs.h \
	full_text_trans.cpp full_text_trans.h

if USE_SYSTEM_SIGCPP
LOCAL_SIGCPP_INCLUDE =
else
LOCAL_SIGCPP_INCLUDE = -I$(top_srcdir)/src -I$(top_srcdir)/src/sigc++config
endif

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_builddir) -I$(top_srcdir) $(LOCAL_SIGCPP_INCLUDE)
