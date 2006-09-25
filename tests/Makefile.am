noinst_PROGRAMS = t_config_file t_dict t_fuzzy t_query t_lookupdata \
	t_convert_old_ini t_dict_client t_articleview t_xml
EXTRA_DIST = sample1.ifo sample1.idx sample1.dict

if GNOME_SUPPORT
GCONF_MODULE_HPP=$(top_builddir)/src/gconf_file.hpp 
GCONF_MODULE_CPP=$(top_builddir)/src/gconf_file.cpp
endif

t_config_file_SOURCES = t_config_file.cpp \
	$(top_builddir)/src/config_file.hpp \
	$(top_builddir)/src/inifile.hpp $(top_builddir)/src/inifile.cpp \
	$(GCONF_MODULE_HPP) $(GCONF_MODULE_CPP)

t_dict_SOURCES = t_dict.cpp
t_dict_DEPENDENCIES = $(top_builddir)/src/lib/libstardict.a

t_dict_client_SOURCES = t_dict_client.cpp
t_dict_client_DEPENDENCIES = $(top_builddir)/src/lib/libstardict.a \
	$(top_builddir)/src/sigc++/libsigc.a

t_fuzzy_SOURCES = t_fuzzy.cpp
t_fuzzy_DEPENDENCIES = $(top_builddir)/src/lib/libstardict.a

t_query_SOURCES = t_query.cpp
t_query_DEPENDENCIES = $(top_builddir)/src/lib/libstardict.a

t_lookupdata_SOURCES = t_lookupdata.cpp
t_lookupdata_DEPENDENCIES = $(top_builddir)/src/lib/libstardict.a

t_convert_old_ini_SOURCES = t_convert_old_ini.cpp \
	$(top_builddir)/src/config_file.hpp \
	$(top_builddir)/src/inifile.hpp $(top_builddir)/src/inifile.cpp

t_articleview_SOURCES = t_articleview.cpp \
	$(top_builddir)/src/articleview.cpp $(top_builddir)/src/articleview.h \
	$(top_builddir)/src/pangoview.cpp $(top_builddir)/src/pangoview.h \
	$(top_builddir)/src/gtktextviewpango.cpp \
	$(top_builddir)/src/gtktextviewpango.h \
	$(top_builddir)/src/utils.cpp $(top_builddir)/src/utils.h \
	$(top_builddir)/src/config_file.hpp \
	$(top_builddir)/src/inifile.hpp $(top_builddir)/src/inifile.cpp \
	$(top_builddir)/src/conf.cpp $(top_builddir)/src/conf.h \
	$(GCONF_MODULE_HPP) $(GCONF_MODULE_CPP)

t_xml_SOURCES = t_xml.cpp \
	$(top_builddir)/src/utils.cpp $(top_builddir)/src/utils.h

LDADD = @STARDICT_LIBS@ $(top_builddir)/src/lib/libstardict.a \
	$(top_builddir)/src/sigc++/libsigc.a
INCLUDES = @STARDICT_CFLAGS@  -I$(top_builddir) -I$(top_builddir)/src -I$(top_builddir)/src/lib

TESTS = \
	t_config_file t_convert_old_ini t_dict t_query t_lookupdata \
	t_articleview t_xml
