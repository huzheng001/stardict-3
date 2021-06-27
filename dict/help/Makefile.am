## Process this file with automake to produce Makefile.in.
## always include gnome-doc-utils.make because we need to generate DocBook files in linguas dirs
#include $(top_srcdir)/gnome-doc-utils.make
if GNOME_SUPPORT
GNOME_DIST_HOOK = doc-dist-hook
else
GNOME_DIST_HOOK =
endif

dist-hook: $(GNOME_DIST_HOOK)

DOC_MODULE = stardict
DOC_ENTITIES = legal.xml
DOC_INCLUDES =
DOC_FIGURES = \
	figures/stardict_float_window.png	\
	figures/stardict_main_window.png	\
	figures/stardict_notification_area.png	\
	figures/stardict_full_text_translation.png	\
	figures/stardict_windows_console.png	\
	figures/stardict_manage_dicts_add_dict.png	\
	figures/stardict_manage_dicts_dict_list.png	\
	figures/stardict_manage_dicts_manage_dict.png	\
	figures/stardict_manage_dicts_network_dicts_browse.png	\
	figures/stardict_manage_dicts_network_dicts.png	\
	figures/stardict_manage_dicts_tree_dict.png	\
	figures/acrobat_toolbar.png	\
	figures/portable_apps_menu.png

DOC_LINGUAS = mk nl ru uk zh_CN zh_TW


STARDICT_ALL_LINGUAS = C $(DOC_LINGUAS)
STARDICT_DB2HTML ?= `$(PKG_CONFIG) --variable db2html gnome-doc-utils`
STARDICT_SYSICONDIR ?= `$(PKG_CONFIG) --variable icondir gnome-doc-utils`/hicolor/48x48/status
STARDICT_ICONDIR = icons
STARDICT_HTML_DIRS = $(foreach lc,$(STARDICT_ALL_LINGUAS),$(lc)/html)
STARDICT_ADMON_ICONS = \
	admon-caution.png	\
	admon-important.png	\
	admon-note.png		\
	admon-bug.png		\
	admon-tip.png		\
	admon-warning.png

## How does StarDict knows where to search for html doc?
## src/Makefile.am defines STARDICT_DATA_DIR macro that is passed to the compiler.
## We put html doc under the STARDICT_DATA_DIR directory.
STARDICT_DATA_DIR = $(datadir)/stardict
STARDICT_HTML_INSTALL_DIR = $(STARDICT_DATA_DIR)/help

.PHONY: stardict-html
stardict-html: $(STARDICT_HTML_DIRS)

## cannot add $(foreach ent,$(DOC_ENTITIES),%/$(ent)) in prerequisits it is for C only
$(STARDICT_HTML_DIRS): %/html : %/$(DOC_MODULE).xml $(foreach fig,$(DOC_FIGURES),%/$(fig))
	lc=$(patsubst %/html,%,$@); \
	htmldir=$(srcdir)/$${lc}/html; \
	chunk_basename=index; \
	test -d $${htmldir} && rm -rf $${htmldir}; \
	$(mkinstalldirs) $${htmldir}; \
	xsltproc -o $${htmldir}/$${chunk_basename}.html --xinclude --param db.chunk.chunk_top "false()" --stringparam db.chunk.basename "$${chunk_basename}" --stringparam db.chunk.extension ".html" --stringparam theme.icon.admon.path "${STARDICT_ICONDIR}/" ${STARDICT_DB2HTML} $<; \
	for fig in $(DOC_FIGURES); do \
		figdir=`echo $$fig | sed -e 's/^\(.*\/\).*/\1/' -e '/\//!s/.*//' -e 's/\/$$//'`; \
		if test ! -d "$${htmldir}/$${figdir}"; then \
			$(mkinstalldirs) "$${htmldir}/$${figdir}"; \
		fi; \
		$(INSTALL_DATA) $(srcdir)/$${lc}/$${fig} $${htmldir}/$${fig}; \
	done; \
	for icon in $(STARDICT_ADMON_ICONS); do \
		if test ! -d "$${htmldir}/$(STARDICT_ICONDIR)"; then \
			$(mkinstalldirs) "$${htmldir}/$(STARDICT_ICONDIR)"; \
		fi; \
		$(INSTALL_DATA) $(STARDICT_SYSICONDIR)/$${icon} $${htmldir}/$(STARDICT_ICONDIR)/$${icon}; \
	done;

maintainer-clean-local: stardict-maintainer-clean

## The html documentation is part of the distribution, it should not be removed by ordinary users.
## That is neither clean nor distclean removes the html documentation.
.PHONY: stardict-maintainer-clean
stardict-maintainer-clean:
	for lc in $(STARDICT_ALL_LINGUAS); do \
		htmldir=$(srcdir)/$${lc}/html; \
		rm -rf $${htmldir}; \
	done;

dist-hook: stardict-dist

.PHONY: stardict-dist
stardict-dist: $(STARDICT_HTML_DIRS)
	for lc in $(STARDICT_ALL_LINGUAS); do \
		htmldir=$(srcdir)/$${lc}/html; \
		for file in `find $${htmldir} -type f -print`; do \
			relfile=$${file#$${htmldir}/}; \
			reldir=`echo $$relfile | sed -e 's/^\(.*\/\).*/\1/' -e '/\//!s/.*//' -e 's/\/$$//'`; \
			targetdir="$(distdir)/$${lc}/html/$${reldir}"; \
			if test ! -d "$${targetdir}"; then \
				$(mkinstalldirs) $${targetdir}; \
			fi; \
			$(INSTALL_DATA) $${file} $(distdir)/$${lc}/html/$${relfile}; \
		done; \
	done;

install-data-local: stardict-install-html

.PHONY: stardict-install-html
if GNOME_SUPPORT
stardict-install-html:
else
stardict-install-html:
	for lc in $(STARDICT_ALL_LINGUAS); do \
		install_dir=$(DESTDIR)$(STARDICT_HTML_INSTALL_DIR)/$${lc}; \
		htmldir=$(srcdir)/$${lc}/html; \
		for file in `find $${htmldir} -type f -print`; do \
			relfile=$${file#$${htmldir}/}; \
			reldir=`echo $$relfile | sed -e 's/^\(.*\/\).*/\1/' -e '/\//!s/.*//' -e 's/\/$$//'`; \
			targetdir="$${install_dir}/$${reldir}"; \
			if test ! -d "$${targetdir}"; then \
				$(mkinstalldirs) $${targetdir}; \
			fi; \
			$(INSTALL_DATA) $${file} $${install_dir}/$${relfile}; \
		done; \
	done;
endif

uninstall-local: stardict-uninstall-html

.PHONY: stardict-uninstall-html
if GNOME_SUPPORT
stardict-uninstall-html:
else
stardict-uninstall-html:
	for lc in $(STARDICT_ALL_LINGUAS); do \
		install_dir=$(DESTDIR)$(STARDICT_HTML_INSTALL_DIR)/$${lc}; \
		rm -rf $${install_dir}; \
	done;
endif

.PHONY: update-po
update-po:
	for lc in $(DOC_LINGUAS); do \
		$(_xml2po) -u $(srcdir)/$${lc}/$${lc}.po $(srcdir)/C/$(DOC_MODULE).xml; \
	done;
