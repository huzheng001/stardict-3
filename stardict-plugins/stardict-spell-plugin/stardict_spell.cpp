#include "stardict_spell.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <enchant.h>
#include <pango/pango.h>
#include <string>
#include <vector>

static const StarDictPluginSystemInfo *plugin_info = NULL;
static EnchantBroker *broker = NULL;
static EnchantDict *dict = NULL;
static PangoLayout *layout = NULL;

static char *build_dictdata(char type, const char *definition)
{
	size_t len = strlen(definition);
	guint32 size;
	size = sizeof(char) + len + 1;
	char *data = (char *)g_malloc(sizeof(guint32) + size);
	char *p = data;
	*((guint32 *)p)= size;
	p += sizeof(guint32);
	*p = type;
	p++;
	memcpy(p, definition, len+1);
	return data;
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
			if (enchant_dict_check(dict, spellword, strlen(spellword)) <= 0)
				misspelled = false;
			else
				misspelled = true;
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
		gchar *escape;
		for (size_t i = 0; i < len; i++) {
			if (insert_tags[i] == 1) {
				underline_str += "<span underline=\"error\" underline_color=\"#FF0000\">";
			} else if (insert_tags[i] == 2) {
				underline_str += "</span>";
			}
			escape = g_markup_escape_text(&(text[i]), 1);
			underline_str += escape;
			g_free(escape);
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
		suggestion = enchant_dict_suggest(dict, (*iter).c_str(), -1, NULL);
		if (suggestion ==NULL)
			continue;
		std::string definition;
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
		result.push_back(std::pair<char *, char *>(g_strdup((*iter).c_str()), build_dictdata('x', definition.c_str())));
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
			(*ppppWordData)[0][0] =  build_dictdata('g', underline_str.c_str());
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

bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Spell plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_VIRTUALDICT;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("Spell Check"), _("Spell check virtual dictionary."), _("Spell check the input words and show the correct suggestion."));
	plugin_info = obj->plugin_info;

	return false;
}

void stardict_plugin_exit(void)
{
	if (broker) {
		if (dict)
			enchant_broker_free_dict(broker, dict);
		enchant_broker_free(broker);
	}
	if (layout) {
		g_object_unref(layout);
	}
}

bool stardict_virtualdict_plugin_init(StarDictVirtualDictPlugInObject *obj)
{
	obj->lookup_func = lookup;
	obj->is_instant = true;
	obj->dict_name = _("Spell Suggestion");
	broker = enchant_broker_init();
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
		g_print("Error, no spell dictionary available!\n");
		return true;
	}
	layout = pango_layout_new(gtk_widget_get_pango_context(plugin_info->mainwin));
	g_print(_("Spell plug-in loaded.\n"));
	return false;
}
