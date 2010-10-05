## Process this file with automake to produce Makefile.in

SUBDIRS = m4 src stardict-plugins tests pixmaps po help data doc \
	debian-upstream msvc_2005
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	ChangeLog  		\
	README 			\
	README.SVN		\
	INSTALL			\
	TODO 			\
	NEWS			\
	autogen.sh		\
	stardict.spec.in stardict-withoutgnome.spec.in \
	dictionary.spec	\
	treedict.spec	\
	stardict.ico stardict-uninst.ico stardict-installer.nsi stardict-prepare-installer.js \
	omf.make		\
	intltool-extract.in 	\
	intltool-merge.in 	\
	intltool-update.in	\
	stardictrc.rc	\
	gnome-doc-utils.make \
	config-custom.h

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper --disable-schemas-install
DISTCLEANFILES = intltool-extract intltool-merge intltool-update
