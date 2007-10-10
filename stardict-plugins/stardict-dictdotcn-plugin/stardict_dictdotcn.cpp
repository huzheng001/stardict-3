#include "stardict_dictdotcn.h"
#include <glib/gi18n.h>
#include <string>
#include <list>

#ifdef _WIN32
#include <windows.h>

static char *strcasestr (const char *phaystack, const char *pneedle)
{
	register const unsigned char *haystack, *needle;
	register char b, c;

	haystack = (const unsigned char *) phaystack;
	needle = (const unsigned char *) pneedle;

	b = tolower(*needle);
	if (b != '\0') {
		haystack--;             /* possible ANSI violation */
		do {
			c = *++haystack;
			if (c == '\0')
				goto ret0;
		} while (tolower(c) != (int) b);

		c = tolower(*++needle);
		if (c == '\0')
			goto foundneedle;
		++needle;
		goto jin;

		for (;;) {
			register char a;
			register const unsigned char *rhaystack, *rneedle;

			do {
				a = *++haystack;
				if (a == '\0')
					goto ret0;
				if (tolower(a) == (int) b)
					break;
				a = *++haystack;
				if (a == '\0')
					goto ret0;
			shloop:
				;
			}
			while (tolower(a) != (int) b);

		jin:      a = *++haystack;
			if (a == '\0')
				goto ret0;

			if (tolower(a) != (int) c)
				goto shloop;

			rhaystack = haystack-- + 1;
			rneedle = needle;
			a = tolower(*rneedle);

			if (tolower(*rhaystack) == (int) a)
				do {
					if (a == '\0')
						goto foundneedle;
					++rhaystack;
					a = tolower(*++needle);
					if (tolower(*rhaystack) != (int) a)
						break;
					if (a == '\0')
						goto foundneedle;
					++rhaystack;
					a = tolower(*++needle);
				} while (tolower (*rhaystack) == (int) a);

			needle = rneedle;             /* took the register-poor approach */

			if (a == '\0')
				break;
		}
	}
 foundneedle:
	return (char*) haystack;
 ret0:
	return 0;
}
#endif

#define DICTDOTCN "dict.cn"

static const StarDictPluginSystemInfo *plugin_info = NULL;
static const StarDictPluginSystemService *plugin_service;

struct QueryInfo {
	bool ismainwin;
	char *word;
};

static std::list<QueryInfo *> keyword_list;
static bool use_html_or_xml;

static std::string get_cfg_filename()
{
#ifdef _WIN32
	std::string res = g_get_user_config_dir();
	res += G_DIR_SEPARATOR_S "StarDict" G_DIR_SEPARATOR_S "dictdotcn.cfg";
#else
	std::string res;
	gchar *tmp = g_build_filename(g_get_home_dir(), ".stardict", NULL);
	res=tmp;
	g_free(tmp);
	res += G_DIR_SEPARATOR_S "dictdotcn.cfg";
#endif
	return res;
}

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

struct dict_ParseUserData {
	std::string pron;
	std::string def;
	std::string rel;
	std::list<std::pair<std::string, std::string> > sentences;
	std::string orig;
	std::string trans;
};

static void dict_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	dict_ParseUserData *Data = (dict_ParseUserData *)user_data;
	if (strcmp(element, "pron")==0) {
		Data->pron.assign(text, text_len);
	} else if (strcmp(element, "def")==0) {
		Data->def.assign(text, text_len);
	} else if (strcmp(element, "rel")==0) {
		Data->rel.assign(text, text_len);
	} else if (strcmp(element, "orig")==0) {
		Data->orig.assign(text, text_len);
	} else if (strcmp(element, "trans")==0) {
		Data->trans.assign(text, text_len);
	}
}

static void dict_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "sent")==0) {
		dict_ParseUserData *Data = (dict_ParseUserData *)user_data;
		Data->orig.clear();
		Data->trans.clear();
	}
}

static void dict_parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "sent")==0) {
		dict_ParseUserData *Data = (dict_ParseUserData *)user_data;
		Data->sentences.push_back(std::pair<std::string, std::string>(Data->orig, Data->trans));
	}
}

static void on_get_http_response(char *buffer, size_t buffer_len, gpointer userdata)
{
	if (!buffer) {
		return;
	}
	const char *p = g_strstr_len(buffer, buffer_len, "\r\n\r\n");
	if (!p) {
		return;
	}
	p += 4;
	QueryInfo *qi = (QueryInfo *)userdata;
	NetDictResponse *resp = new NetDictResponse;
	resp->bookname = _("Dict.cn");
	resp->word = qi->word; // So neen't free qi->word;
	if (use_html_or_xml) {
		std::string charset;
		char *p3 = g_strstr_len(p, buffer_len - (p - buffer), "charset=");
		if (p3) {
			p3 += sizeof("charset=") -1;
			char *p4 = strchr(p3, '\"');
			if (p4) {
				charset.assign(p3, p4-p3);
			}
		}
		gchar *content;
		if (charset.empty()) {
			content = NULL;
		} else {
			content = g_convert(p, buffer_len - (p - buffer), "UTF-8", charset.c_str(), NULL, NULL, NULL);
			p = content;
		}
		resp->data = NULL;
		if (p) {
			const char *body = strcasestr(p, "<body");
			if (body) {
				const char *body_end = strcasestr(p, "</body>");
				if (body_end) {
					body_end += sizeof("</body>") -1;
					std::string html(body, body_end - body);
					resp->data = build_dictdata('h', html.c_str());
				}
			}
		}
		g_free(content);
	} else {
		const char *xml = g_strstr_len(p, buffer_len - (p - buffer), "<dict>");
		if (!xml) {
			return;
		}
		const char *xml_end = g_strstr_len(xml+6, buffer_len - (xml+6 - buffer), "</dict>");
		if (!xml_end) {
			return;
		}
		xml_end += 7;
		dict_ParseUserData Data;
		GMarkupParser parser;
		parser.start_element = dict_parse_start_element;
		parser.end_element = dict_parse_end_element;
		parser.text = dict_parse_text;
		parser.passthrough = NULL;
		parser.error = NULL;
		GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
		g_markup_parse_context_parse(context, xml, xml_end - xml, NULL);
		g_markup_parse_context_end_parse(context, NULL);
		g_markup_parse_context_free(context);
		if (Data.def == "Not Found") {
			resp->data = NULL;
		} else {
			std::string definition;
			if (!Data.pron.empty()) {
				definition += "[";
				definition += Data.pron;
				definition += "]\n";
			}
			definition += Data.def;
			if (!Data.rel.empty()) {
				definition += "\n";
				definition += Data.rel;
			}
			if (!Data.sentences.empty()) {
				definition += "\n\n例句与用法:";
				int index = 1;
				char *tmp_str;
				for (std::list<std::pair<std::string, std::string> >::iterator i = Data.sentences.begin(); i != Data.sentences.end(); ++i) {
					tmp_str = g_strdup_printf("\n%d. %s\n   %s", index, i->first.c_str(), i->second.c_str());
					definition += tmp_str;
					g_free(tmp_str);
					index++;
				}
			}
			resp->data = build_dictdata('m', definition.c_str());
		}
	}
	plugin_service->netdict_save_cache_resp(DICTDOTCN, qi->word, resp);
	plugin_service->show_netdict_resp(resp, qi->ismainwin);
	delete qi;
	keyword_list.remove(qi);
}

static void lookup(const char *word, bool ismainwin)
{
	std::string file;
	if (use_html_or_xml) {
		file = "/mini.php?q=";
	} else {
		file = "/ws.php?utf8=true&q=";
	}
	char *eword = plugin_service->encode_uri_string(word);
	file += eword;
	g_free(eword);
	gchar *keyword = g_strdup(word);
	QueryInfo *qi = new QueryInfo;
	qi->ismainwin = ismainwin;
	qi->word = keyword;
	keyword_list.push_back(qi);
	plugin_service->send_http_request("dict.cn", file.c_str(), on_get_http_response, qi);
}

static void configure()
{
	GtkWidget *window = gtk_dialog_new_with_buttons(_("Dict.cn configuration"), GTK_WINDOW(plugin_info->pluginwin), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *vbox = gtk_vbox_new(false, 5);
	GtkWidget *xml_button = gtk_radio_button_new_with_label(NULL, _("Query by XML API."));
	gtk_box_pack_start(GTK_BOX(vbox), xml_button, false, false, 0);
	GtkWidget *html_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(xml_button), _("Query by HTML API."));
	gtk_box_pack_start(GTK_BOX(vbox), html_button, false, false, 0);
	if (use_html_or_xml) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(html_button), true);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xml_button), true);
	}
	gtk_widget_show_all(vbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(window)->vbox), vbox);
	gtk_dialog_run(GTK_DIALOG(window));
	gboolean new_use_html_or_xml = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(html_button));
	if (new_use_html_or_xml != use_html_or_xml) {
		use_html_or_xml = new_use_html_or_xml;
		const char *tmp;
		if (use_html_or_xml) {
			tmp = "true";
		} else {
			tmp = "false";
		}
		gchar *data = g_strdup_printf("[dictdotcn]\nuse_html_or_xml=%s\n", tmp);
		std::string res = get_cfg_filename();
		g_file_set_contents(res.c_str(), data, -1, NULL);
		g_free(data);
	}
	gtk_widget_destroy (window);
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Dict.cn plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_NETDICT;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("Dict.cn"), _("Dict.cn network dictionary."), _("Query result from Dict.cn website."));
	obj->configure_func = configure;
	plugin_info = obj->plugin_info;
	plugin_service = obj->plugin_service;
	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
	for (std::list<QueryInfo *>::iterator i = keyword_list.begin(); i != keyword_list.end(); ++i) {
		g_free((*i)->word);
		delete *i;
	}
}

DLLIMPORT bool stardict_netdict_plugin_init(StarDictNetDictPlugInObject *obj)
{
	std::string res = get_cfg_filename();
	if (!g_file_test(res.c_str(), G_FILE_TEST_EXISTS)) {
		g_file_set_contents(res.c_str(), "[dictdotcn]\nuse_html_or_xml=false\n", -1, NULL);
	}
	GKeyFile *keyfile = g_key_file_new();
	g_key_file_load_from_file(keyfile, res.c_str(), G_KEY_FILE_NONE, NULL);
	GError *err = NULL;
	use_html_or_xml = g_key_file_get_boolean(keyfile, "dictdotcn", "use_html_or_xml", &err);
	if (err) {
		g_error_free (err);
		use_html_or_xml = false;
	}
	g_key_file_free(keyfile);
	obj->lookup_func = lookup;
	obj->dict_name = _("Dict.cn");
	obj->dict_cacheid = DICTDOTCN;
	g_print(_("Dict.cn plug-in loaded.\n"));
	return false;
}

#ifdef _WIN32
BOOL APIENTRY DllMain (HINSTANCE hInst     /* Library instance handle. */ ,
                       DWORD reason        /* Reason this function is being called. */ ,
                       LPVOID reserved     /* Not used. */ )
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        break;

      case DLL_PROCESS_DETACH:
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    /* Returns TRUE on success, FALSE on failure */
    return TRUE;
}
#endif
