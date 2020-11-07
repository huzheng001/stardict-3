## Process this file with automake to produce Makefile.in

SUBDIRS = m4 src stardict-plugins tests pixmaps po help data doc \
    debian-upstream \
    mac_dist \
    msvc_2008
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	TODO 			\
	INSTALL			\
	autogen.sh		\
	stardict.spec.in stardict-withoutgnome.spec.in \
	dictionary.spec	\
	treedict.spec	\
	stardict-installer.nsi stardict-prepare-installer.js \
	omf.make		\
	intltool-extract.in 	\
	intltool-merge.in 	\
	intltool-update.in	\
	gnome-doc-utils.make

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper --disable-schemas-install
DISTCLEANFILES = intltool-extract intltool-merge intltool-update
