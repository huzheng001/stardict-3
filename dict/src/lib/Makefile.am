COMMONLIB_CPPFLAGS = -I$(top_srcdir)/$(COMMONLIB_INCLUDE_DIR)
COMMONLIB_LIB = $(top_builddir)/$(COMMONLIB_LIBRARY)

EXTRA_DIST = dict_client.cpp dict_client.h kmp.cpp kmp.h

noinst_LTLIBRARIES = libstardict.la

libstardict_la_SOURCES = \
	dictziplib.cpp dictziplib.h	\
	edit-distance.cpp edit-distance.h	\
	mapfile.h file-utils.h	\
	m_ctype.h	\
	ctype-mb.cpp ctype-utf8.cpp ctype-uca.cpp	\
	collation.cpp collation.h \
	dictbase.h dictbase.cpp \
	stddict.cpp stddict.h \
	storage.cpp storage.h storage_impl.h	\
	treedict.cpp treedict.h	\
	md5.c md5.h	\
	rsa.cpp rsa.h	\
	stardict_client.cpp stardict_client.h \
	sockets.cpp sockets.h \
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
	full_text_trans.cpp full_text_trans.h \
	verify_dict.cpp verify_dict.h \
	dictitemid.h

libstardict_la_LIBADD = $(COMMONLIB_LIB)

if USE_SYSTEM_SIGCPP
LOCAL_SIGCPP_INCLUDE =
else
LOCAL_SIGCPP_INCLUDE = -I$(top_srcdir)/src -I$(top_srcdir)/src/sigc++config
endif

AM_CPPFLAGS = @STARDICT_CFLAGS@ -I$(top_builddir) -I$(top_srcdir) $(LOCAL_SIGCPP_INCLUDE) $(COMMONLIB_CPPFLAGS)
