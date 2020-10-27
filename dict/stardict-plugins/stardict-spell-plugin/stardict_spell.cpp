/*
 * Copyright 2011 kubtek <kubtek@mail.com>
 *
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stardict_spell.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <enchant.h>
#include <pango/pango.h>
#include <cstring>
#include <string>
#include <vector>
#include <string.h>

static const StarDictPluginSystemInfo *plugin_info = NULL;
static const StarDictPluginSystemService *plugin_service;
static IAppDirs* gpAppDirs = NULL;

static EnchantBroker *broker = NULL;
static std::list<EnchantDict *> dictlist;
static PangoLayout *layout = NULL;
static gboolean use_custom;
static std::string custom_langs;


/* concatenate path1 and path2 inserting a path separator in between if needed. */
static std::string build_path(const std::string& path1, const std::string& path2)
{
	std::string res;
	res.reserve(path1.length() + 1 + path2.length());
	res = path1;
	if(!res.empty() && res[res.length()-1] != G_DIR_SEPARATOR)
		res += G_DIR_SEPARATOR_S;
	if(!path2.empty() && path2[0] == G_DIR_SEPARATOR)
		res.append(path2, 1, std::string::npos);
	else
		res.append(path2);
	return res;
}

static std::string get_cfg_filename()
{
	return build_path(gpAppDirs->get_user_config_dir(), "spell.cfg");
}

static void stardict_strsplit_utf8(const gchar *text, gchar ***set, gint **starts, gint **ends)
{
	PangoLogAttr  *log_attrs;
	gint           n_attrs, n_strings, i, j;

	pango_layout_get_log_attrs(layout, &log_attrs, &n_attrs);

	/* Find how many words we have */
	n_strings = 0;
	for (i = 0; i < n_attrs; i++)
		if (log_attrs[i].is_word_start)
			n_strings++;

	*set    = g_new0(gchar *, n_strings + 1);
	*starts = g_new0(gint, n_strings);
	*ends   = g_new0(gint, n_strings);

	/* Copy out strings */
	for (i = 0, j = 0; i < n_attrs; i++) {
		if (log_attrs[i].is_word_start) {
			gint cend, bytes;
			gchar *start;

			/* Find the end of this string */
			cend = i;
			while (!(log_attrs[cend].is_word_end))
				cend++;

			/* Copy sub-string */
			start = g_utf8_offset_to_pointer(text, i);
			bytes = (gint) (g_utf8_offset_to_pointer(text, cend) - start);
			(*set)[j]    = g_new0(gchar, bytes + 1);
			(*starts)[j] = (gint) (start - text);
			(*ends)[j]   = (gint) (start - text + bytes);
			g_utf8_strncpy((*set)[j], start, cend - i);

			/* Move on to the next word */
			j++;
		}
	}

	g_free (log_attrs);
}

static void lookup(const char *text, char ***pppWord, char ****ppppWordData)
{
	size_t len = strlen(text);
	pango_layout_set_text(layout, text, len);
	gchar **words;
	gint *word_starts;
	gint *word_ends;
	stardict_strsplit_utf8(text, &words, &word_starts, &word_ends);
	std::list<std::string> misspelled_wordlist;
	gchar *spellword;
	int start, end;
	bool misspelled;
	int *insert_tags = (int *)g_malloc0(sizeof(int)*(len +1));
	int n_words;
	for (n_words = 0; words[n_words]; n_words++) {
		if (words[n_words][0] == '\0')
			continue;
		start = word_starts[n_words];
		end = word_ends[n_words];
		if (start == end)
			continue;
		spellword = g_new0(gchar, end - start + 2);
		g_strlcpy(spellword, text + start, end - start + 1);
		if (g_unichar_isalpha(*spellword) == FALSE) {
			/* We only want to check words */
			misspelled = false;
		} else {
			misspelled = true;
			size_t wordlen = strlen(spellword);
			for (std::list<EnchantDict *>::iterator iter = dictlist.begin(); iter != dictlist.end(); ++iter) {
				if (enchant_dict_check(*iter, spellword, wordlen) <= 0) {
					misspelled = false;
					break;
				}
			}
		}
		if (misspelled) {
			misspelled_wordlist.push_back(spellword);
			insert_tags[start] = 1;
			insert_tags[end] = 2;
		}
		g_free(spellword);
	}
	std::string underline_str;
	int n_misspelled = misspelled_wordlist.size();
	if (n_misspelled !=0 && n_words != 1) {
		underline_str += "<big>";
		for (size_t i = 0; i < len; i++) {
			if (insert_tags[i] == 1) {
				underline_str += "<span underline=\"error\" underline_color=\"#FF0000\">";
			} else if (insert_tags[i] == 2) {
				underline_str += "</span>";
			}
			switch (text[i]) {
				case '&':
					underline_str += "&amp;";
					break;
				case '<':
					underline_str += "&lt;";
					break;
				case '>':
					underline_str += "&gt;";
					break;
				case '\'':
					underline_str += "&apos;";
					break;
				case '"':
					underline_str += "&quot;";
					break;
				default:
					underline_str += text[i];
			}
		}
		if (insert_tags[len] == 2)
			underline_str += "</span>";
		underline_str += "</big>";
	}
	g_free(insert_tags);
	g_strfreev(words);
	g_free(word_starts);
	g_free(word_ends);

	std::vector< std::pair<char *, char *> > result;
	char **suggestion;
	for (std::list<std::string>::iterator iter = misspelled_wordlist.begin(); iter != misspelled_wordlist.end(); ++iter) {
		std::list<char **> suggestions;
		for (std::list<EnchantDict *>::iterator i = dictlist.begin(); i != dictlist.end(); ++i) {
			suggestion = enchant_dict_suggest(*i, (*iter).c_str(), -1, NULL);
			if (suggestion)
				suggestions.push_back(suggestion);
		}
		if (suggestions.empty())
			continue;
		std::string definition;
		for (std::list<char **>::iterator it = suggestions.begin(); it != suggestions.end(); ++it) {
			if (it != suggestions.begin()) {
				definition += "\n\n";
			}
			suggestion = *it;
			definition += "<kref>";
			definition += suggestion[0];
			definition += "</kref>";
			int i = 1;
			while (suggestion[i]) {
				definition += "\t<kref>";
				definition += suggestion[i];
				definition += "</kref>";
				i++;
			}
		}
		result.push_back(std::pair<char *, char *>(g_strdup((*iter).c_str()), plugin_service->build_dictdata('x', definition.c_str())));
	}
	if (result.empty()) {
		*pppWord = NULL;
	} else {
		int head;
		if (underline_str.empty())
			head = 0;
		else
			head = 1;
		*pppWord = (gchar **)g_malloc(sizeof(gchar *)*(result.size()+1+head));
		*ppppWordData = (gchar ***)g_malloc(sizeof(gchar **)*(result.size()+head));
		if (head) {
			(*pppWord)[0] = g_strdup(text);
			(*ppppWordData)[0] = (gchar **)g_malloc(sizeof(gchar *)*2);
			(*ppppWordData)[0][0] =  plugin_service->build_dictdata('g', underline_str.c_str());
			(*ppppWordData)[0][1] = NULL;
		}
		for (std::vector< std::pair<char *, char *> >::size_type i = 0; i< result.size(); i++) {
			(*pppWord)[i+head] = result[i].first;
			(*ppppWordData)[i+head] = (gchar **)g_malloc(sizeof(gchar *)*2);
			(*ppppWordData)[i+head][0] = result[i].second;
			(*ppppWordData)[i+head][1] = NULL;
		}
		(*pppWord)[result.size()+head] = NULL;
	}
}

static bool load_custom_langs()
{
	for (std::list<EnchantDict *>::iterator i = dictlist.begin(); i != dictlist.end(); ++i) {
		enchant_broker_free_dict(broker, *i);
	}
	dictlist.clear();
	std::list<std::string> langlist;
	std::string lang;
	const gchar *p = custom_langs.c_str();
	const gchar *p1;
	while (true) {
		p1 = strchr(p, ' ');
		if (!p1)
			break;
		lang.assign(p, p1-p);
		langlist.push_back(lang);
		p = p1 + 1;
	}
	lang = p;
	langlist.push_back(lang);
	EnchantDict *dict;
	for (std::list<std::string>::iterator i = langlist.begin(); i != langlist.end(); ++i) {
		dict = enchant_broker_request_dict(broker, i->c_str());
		if (dict) {
			dictlist.push_back(dict);
		} else {
			g_print(_("Warning: failure when requesting a spellchecking dictionary for %s language.\n"), i->c_str());
		}
	}
	if (dictlist.empty()) {
		enchant_broker_free(broker);
		broker = NULL;
		g_print(_("Error, no spellchecking dictionary available!\n"));
		return true;
	} else {
		return false;
	}
}

static bool load_auto_lang()
{
	for (std::list<EnchantDict *>::iterator i = dictlist.begin(); i != dictlist.end(); ++i) {
		enchant_broker_free_dict(broker, *i);
	}
	dictlist.clear();
	bool no_dict = false;
	const gchar* const *languages = g_get_language_names();
	int i = 0;
	while (true) {
		if (languages[i]==NULL) {
			no_dict = true;
			break;
		}
		if (strchr(languages[i], '.') == NULL) {
			if (enchant_broker_dict_exists(broker, languages[i]))
				break;
		}
		i++;
	}
	bool found;
	EnchantDict *dict;
	if (no_dict) {
		if (enchant_broker_dict_exists(broker, "en_US") && ((dict = enchant_broker_request_dict(broker, "en_US"))!=NULL))
			found = true;
		else
			found = false;
	} else {
		if ((dict = enchant_broker_request_dict(broker, languages[i]))!=NULL)
			found = true;
		else
			found = false;
	}
	if (!found) {
		enchant_broker_free(broker);
		broker = NULL;
		g_print(_("Error, no spellchecking dictionary available!\n"));
		return true;
	} else {
		dictlist.push_back(dict);
		return false;
	}
}

static void on_use_custom__ckbutton_toggled(GtkToggleButton *button, GtkWidget *hbox)
{
	gtk_widget_set_sensitive(hbox, gtk_toggle_button_get_active(button));
}

static void configure()
{
	GtkWidget *window = gtk_dialog_new_with_buttons(_("Spell check configuration"), GTK_WINDOW(plugin_info->pluginwin), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#else
	GtkWidget *vbox = gtk_vbox_new(false, 5);
#endif
	GtkWidget *check_button = gtk_check_button_new_with_mnemonic(_("_Use custom languages."));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), use_custom);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, false, false, 0);
	GtkWidget *label = gtk_label_new(_("For example: \"en_US de\""));
	gtk_box_pack_start(GTK_BOX(vbox), label, false, false, 0);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	GtkWidget *hbox = gtk_hbox_new(FALSE, 5);
#endif
	gtk_widget_set_sensitive(hbox, use_custom);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(on_use_custom__ckbutton_toggled), hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	label = gtk_label_new(_("Custom languages:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	GtkWidget *entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), custom_langs.c_str());
	gtk_box_pack_start(GTK_BOX(hbox), entry, false, false, 0);
	gtk_widget_show_all(vbox);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG(window))), vbox);
	gtk_dialog_run(GTK_DIALOG(window));
	gboolean new_use_custom = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button));
	bool cfgchanged = false;
	if (new_use_custom != use_custom) {
		cfgchanged = true;
		use_custom = new_use_custom;
		custom_langs = gtk_entry_get_text(GTK_ENTRY(entry));
		if (use_custom) {
			load_custom_langs();
		} else {
			load_auto_lang();
		}
	} else {
		if (use_custom == true) {
			const gchar *str = gtk_entry_get_text(GTK_ENTRY(entry));
			if (custom_langs != str) {
				cfgchanged = true;
				custom_langs = str;
				load_custom_langs();
			}
		}
	}
	if (cfgchanged) {
		const char *tmp;
		if (use_custom)
			tmp = "true";
		else
			tmp = "false";
		gchar *data = g_strdup_printf("[spell]\nuse_custom=%s\ncustom_langs=%s\n", tmp, custom_langs.c_str());
		std::string res = get_cfg_filename();
		g_file_set_contents(res.c_str(), data, -1, NULL);
		g_free(data);
	}
	gtk_widget_destroy (window);
}

bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs)
{
	g_debug(_("Loading Spelling plug-in..."));
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print(_("Error: Spell plugin version doesn't match!\n"));
		return true;
	}
	obj->type = StarDictPlugInType_VIRTUALDICT;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng001@gmail.com&gt;</author><website>http://stardict-4.sourceforge.net</website></plugin_info>", _("Spell Check"), _("Spell check virtual dictionary."), _("Spell check the input words and show the correct suggestion."));
	obj->configure_func = configure;
	plugin_info = obj->plugin_info;
	plugin_service = obj->plugin_service;
	gpAppDirs = appDirs;

	return false;
}

void stardict_plugin_exit(void)
{
	if (broker) {
		for (std::list<EnchantDict *>::iterator i = dictlist.begin(); i != dictlist.end(); ++i) {
			enchant_broker_free_dict(broker, *i);
		}
		enchant_broker_free(broker);
	}
	if (layout) {
		g_object_unref(layout);
	}
	gpAppDirs = NULL;
}

bool stardict_virtualdict_plugin_init(StarDictVirtualDictPlugInObject *obj)
{
	obj->lookup_func = lookup;
	obj->dict_name = _("Spelling Suggestion");
	broker = enchant_broker_init();
	layout = pango_layout_new(gtk_widget_get_pango_context(plugin_info->mainwin));

	std::string res = get_cfg_filename();
	if (!g_file_test(res.c_str(), G_FILE_TEST_EXISTS)) {
		g_file_set_contents(res.c_str(), "[spell]\nuse_custom=false\ncustom_langs=\n", -1, NULL);
	}
	GKeyFile *keyfile = g_key_file_new();
	g_key_file_load_from_file(keyfile, res.c_str(), G_KEY_FILE_NONE, NULL);
	GError *err = NULL;
	use_custom = g_key_file_get_boolean(keyfile, "spell", "use_custom", &err);
	if (err) {
		g_error_free (err);
		use_custom = false;
	}
	gchar *str = g_key_file_get_string(keyfile, "spell", "custom_langs", NULL);
	if (str) {
		custom_langs = str;
		g_free(str);
	}
	g_key_file_free(keyfile);
	bool failed;
	if (use_custom && !custom_langs.empty()) {
		failed = load_custom_langs();
	} else {
		failed = load_auto_lang();
	}
	if (failed)
		return true;
	g_print(_("Spelling plugin loaded.\n"));
	return false;
}
