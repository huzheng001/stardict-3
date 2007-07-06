## Process this file with automake to produce Makefile.in
if USE_SYSTEM_SIGCPP
LOCAL_SIGCPP_DIR =
LOCAL_SIGCPP_LIBFILE =
LOCAL_SIGCPP_INCLUDE =
else
LOCAL_SIGCPP_DIR = sigc++ sigc++config
LOCAL_SIGCPP_LIBFILE = sigc++/libsigc.a
LOCAL_SIGCPP_INCLUDE = -Isigc++config
endif

DIST_SUBDIRS = sigc++ sigc++config lib pixmaps sounds win32 dic treedict
SUBDIRS = $(LOCAL_SIGCPP_DIR) lib pixmaps sounds win32 dic treedict

bin_PROGRAMS = stardict

INCLUDES =							                            \
	$(STARDICT_CFLAGS)						                    \
	$(LOCAL_SIGCPP_INCLUDE)	\
	-DDATADIR=\""$(datadir)"\"	\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"	\
	-DSTARDICT_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DSTARDICT_DATA_DIR=\""$(datadir)/stardict"\"	\
	-DSTARDICT_LIB_DIR=\""$(libdir)/stardict"\"


stardict_DEPENDENCIES = $(LOCAL_SIGCPP_LIBFILE) lib/libstardict.a
stardict_LDFLAGS = 
stardict_LDADD = $(STARDICT_LIBS) $(LOCAL_SIGCPP_LIBFILE) lib/libstardict.a

if !GNOME_SUPPORT
if MAEMO_SUPPORT
CONFIG_FILE_MODULE = gconf_file.cpp gconf_file.hpp
else
CONFIG_FILE_MODULE = inifile.cpp inifile.hpp
endif
else
CONFIG_FILE_MODULE = gconf_file.cpp gconf_file.hpp
IDL_FILE = GNOME_Stardict.idl

IDL_SOURCES = \
	GNOME_Stardict-stubs.c			\
	GNOME_Stardict-common.c			\
	GNOME_Stardict-skels.c			\
	GNOME_Stardict.h		

STARIDCT_APP_SERVER = \
	stardict-application-server.cpp stardict-application-server.h
endif

stardict_SOURCES = 			                \
	$(IDL_SOURCES)			                \
	$(STARIDCT_APP_SERVER)                  \
	iskeyspressed.hpp            \
	$(ISKEYSPRESSED)                        \
	stardict.cpp stardict.h			        \
	mainwin.cpp mainwin.h	                \
	eggtrayicon.c eggtrayicon.h	            \
	docklet.cpp docklet.h	                \
	conf.cpp conf.h	                        \
	skin.cpp skin.h	                        \
	prefsdlg.cpp prefsdlg.h	                \
	dictmanagedlg.cpp dictmanagedlg.h	    \
	pluginmanagedlg.cpp pluginmanagedlg.h	\
	floatwin.cpp floatwin.h	                \
	readword.cpp readword.h	                \
	selection.cpp selection.h	            \
	splash.cpp splash.h		                \
	utils.cpp utils.h                       \
	gtktextviewpango.cpp gtktextviewpango.h \
	pangoview.cpp pangoview.h               \
	articleview.cpp articleview.h           \
	class_factory.cpp class_factory.hpp     \
	config_file.hpp              \
	$(CONFIG_FILE_MODULE)                   \
	x11_iskeyspressed.cpp x11_iskeyspressed.hpp \
	gtk_iskeyspressed.cpp gtk_iskeyspressed.hpp \
	progresswin.cpp progresswin.hpp \
	tray.cpp tray.hpp \
	dictmanage.cpp dictmanage.h	\
	desktop.cpp desktop.hpp

if GNOME_SUPPORT
$(IDL_SOURCES): $(IDL_FILE)
	$(ORBIT_IDL) -I $(LIBBONOBO_IDL) -I $(BONOBO_ACTIVATION_IDL) $(IDL_FILE)

idl_DATA = $(IDL_FILE)
idldir = $(datadir)/idl

EXTRA_DIST = 		\
	$(IDL_FILE)

CLEANFILES = $(IDL_SOURCES)
endif
