## Process this file with automake to produce Makefile.in
COMMONLIB_CPPFLAGS = -I$(top_srcdir)/$(COMMONLIB_INCLUDE_DIR)
COMMONLIB_LIB = $(top_builddir)/$(COMMONLIB_LIBRARY)

if USE_SYSTEM_SIGCPP
LOCAL_SIGCPP_DIR =
LOCAL_SIGCPP_LIBFILE =
LOCAL_SIGCPP_INCLUDE =
else
LOCAL_SIGCPP_DIR = sigc++ sigc++config
LOCAL_SIGCPP_LIBFILE = sigc++/libsigc.a
LOCAL_SIGCPP_INCLUDE = -I$(srcdir) -I$(srcdir)/sigc++config
endif

DIST_SUBDIRS = sigc++ sigc++config lib pixmaps sounds win32 dic treedict skins
SUBDIRS = $(LOCAL_SIGCPP_DIR) lib pixmaps sounds win32 dic treedict skins

bin_PROGRAMS = stardict

AM_CPPFLAGS =							                            \
	-I$(top_builddir)					                            \
	-I$(top_srcdir)						                            \
	$(STARDICT_CFLAGS)						                    \
	$(LOCAL_SIGCPP_INCLUDE)	\
	$(COMMONLIB_CPPFLAGS)	\
	-DSYSTEM_DATA_DIR=\""$(datadir)"\"	\
	-DSYSTEM_ICON_DIR=\""$(datadir)/pixmaps"\"	\
	-DSTARDICT_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DSTARDICT_DATA_DIR=\""$(datadir)/stardict"\"	\
	-DSTARDICT_LIB_DIR=\""$(libdir)/stardict"\"


stardict_DEPENDENCIES = lib/libstardict.la $(LOCAL_SIGCPP_LIBFILE)
stardict_LDFLAGS = 
## place libstardict.la before any system library, otherwise build with --as-needed linker option may fail
stardict_LDADD = lib/libstardict.la $(STARDICT_LIBS) $(LOCAL_SIGCPP_LIBFILE)

if !GNOME_SUPPORT
if MAEMO_SUPPORT
CONFIG_FILE_MODULE = gconf_file.cpp gconf_file.h
else
CONFIG_FILE_MODULE =
endif
else
CONFIG_FILE_MODULE = gconf_file.cpp gconf_file.h
IDL_FILE = GNOME_Stardict.idl

IDL_SOURCES = \
	GNOME_Stardict-stubs.c			\
	GNOME_Stardict-common.c			\
	GNOME_Stardict-skels.c			\
	GNOME_Stardict.h		

STARIDCT_APP_SERVER = \
	stardict-application-server.cpp \
	stardict-application-server.h
endif

if !DARWIN_SUPPORT
KEYPRESS_FILE_MODULE = x11_iskeyspressed.cpp x11_iskeyspressed.h
DOCKLET_FILE_MODULE = docklet.cpp docklet.h
HOTKEY_FILE_MODULE = eggaccelerators.cpp eggaccelerators.h tomboykeybinder.cpp tomboykeybinder.h
endif

stardict_SOURCES = 			                \
	$(IDL_SOURCES)			                \
	$(STARIDCT_APP_SERVER)                  \
	iskeyspressed.h            \
	stardict.cpp stardict.h			        \
	mainwin.cpp mainwin.h	                \
	conf.cpp conf.h	                        \
	skin.cpp skin.h	                        \
	prefsdlg.cpp prefsdlg.h	                \
	dictmanagedlg.cpp dictmanagedlg.h	    \
	pluginmanagedlg.cpp pluginmanagedlg.h	\
	floatwin.cpp floatwin.h	                \
	readword.cpp readword.h	                \
	selection.cpp selection.h	            \
	splash.cpp splash.h		                \
	gtktextviewpango.cpp gtktextviewpango.h \
	pangoview.cpp pangoview.h               \
	articleview.cpp articleview.h           \
	class_factory.cpp class_factory.h     \
	config_file.h              \
	inifile.cpp inifile.h                 \
	$(CONFIG_FILE_MODULE)                   \
	$(KEYPRESS_FILE_MODULE) \
	$(DOCKLET_FILE_MODULE) \
	$(HOTKEY_FILE_MODULE) \
	gtk_iskeyspressed.cpp gtk_iskeyspressed.h \
	progresswin.cpp progresswin.h \
	tray.cpp tray.h \
	dictmanage.cpp dictmanage.h	\
	globalhotkeys.cpp globalhotkeys.h	\
	hotkeyeditor.c hotkeyeditor.h	\
	desktop.cpp desktop.h	\
	log.cpp log.h \
	cmdlineopts.cpp \
	cmdlineopts.h

if GNOME_SUPPORT
$(IDL_SOURCES): $(IDL_FILE)
	$(ORBIT_IDL) -I $(LIBBONOBO_IDL) -I $(BONOBO_ACTIVATION_IDL) $^

idl_DATA = $(IDL_FILE)
idldir = $(datadir)/idl

EXTRA_DIST = 		\
	$(IDL_FILE)

CLEANFILES = $(IDL_SOURCES)
endif
