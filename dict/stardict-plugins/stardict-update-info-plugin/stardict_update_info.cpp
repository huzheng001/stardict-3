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

#include "stardict_update_info.h"
#include <glib/gi18n.h>
#include <cstring>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

static const int my_version_num = 30007000; // As 3,00,07,000, so the version is 3.0.7.0
static int latest_version_num;
static int last_prompt_num;
static std::string version_msg_title;
static std::string version_msg_content;
static std::string latest_news;
static bool show_ads;

static const StarDictPluginSystemInfo *plugin_info = NULL;
static const StarDictPluginSystemService *plugin_service;
static IAppDirs* gpAppDirs = NULL;
#define UTF8_BOM "\xEF\xBB\xBF"

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
	return build_path(gpAppDirs->get_user_config_dir(), "update_info.cfg");
}


static void on_get_http_response(const char *buffer, size_t buffer_len, gpointer userdata);

static void configure()
{
	GtkWidget *window = gtk_dialog_new_with_buttons(_("Update information"), GTK_WINDOW(plugin_info->pluginwin), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#else
	GtkWidget *vbox = gtk_vbox_new(false, 5);
#endif

	std::string content;
	if (latest_version_num > my_version_num) {
		content += _("You are using an old version of StarDict!");
	} else {
		content += _("You are using the newest version of StarDict!");
	}
	content += "\n\n";
	content += _("Latest version information:");
	content += "\n";
	content += version_msg_title;
	content += "\n";
	content += version_msg_content;
	content += "\n\n";
	content += _("Latest news:");
	content += "\n";
	content += latest_news;
	GtkWidget *label = gtk_label_new(content.c_str());
	gtk_label_set_line_wrap(GTK_LABEL(label), true);
	gtk_label_set_selectable(GTK_LABEL(label), true);
	gtk_box_pack_start(GTK_BOX(vbox), label, false, false, 5);


	GtkWidget *check_button = gtk_check_button_new_with_mnemonic(_("_Show advertisements."));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), show_ads);
	gtk_box_pack_start(GTK_BOX(vbox), check_button, false, false, 0);

	gtk_widget_show_all(vbox);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG(window))), vbox);
	gtk_dialog_run(GTK_DIALOG(window));

	gboolean new_show_ads = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button));
	if (new_show_ads != show_ads) {
		show_ads = new_show_ads;

		GKeyFile *keyfile = g_key_file_new();
		g_key_file_set_string(keyfile, "update", "version_msg_title", version_msg_title.c_str());
		g_key_file_set_string(keyfile, "update", "version_msg_content", version_msg_content.c_str());
		g_key_file_set_string(keyfile, "update", "latest_news", latest_news.c_str());
		g_key_file_set_integer(keyfile, "update", "latest_version_num", latest_version_num);
		g_key_file_set_integer(keyfile, "update", "last_prompt_num", last_prompt_num);
		g_key_file_set_boolean(keyfile, "misc", "show_ads", show_ads);
		gsize length;
		gchar *content = g_key_file_to_data(keyfile, &length, NULL);
		std::string res = get_cfg_filename();
		g_file_set_contents(res.c_str(), content, length, NULL);
		g_free(content);

		if (new_show_ads) {
			plugin_service->send_http_request("stardict-4.sourceforge.net", "/UPDATE", on_get_http_response, NULL);
		} else {
			plugin_service->set_news(NULL, NULL);
		}
	}
	gtk_widget_destroy (window);
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs)
{
	g_debug(_("Loading Update info plug-in..."));
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print(_("Error: Update info plugin version doesn't match!\n"));
		return true;
	}
	obj->type = StarDictPlugInType_MISC;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng001@gmail.com&gt;</author><website>http://stardict-4.sourceforge.net</website></plugin_info>", _("Update Info"), _("Update information."), _("Get the update information from the Internet."));
	obj->configure_func = configure;
	plugin_info = obj->plugin_info;
	plugin_service = obj->plugin_service;
	gpAppDirs = appDirs;
	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
	gpAppDirs = NULL;
}

struct updateinfo_ParseUserData {
	std::string locale_name;
	int latest_version_num;
	std::string version_msg_title;
	std::string version_msg_content;
	std::string latest_news;
	std::string links;
};

static void updateinfo_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	updateinfo_ParseUserData *Data = (updateinfo_ParseUserData *)user_data;
	if (strcmp(element, "latest_version_num")==0) {
		std::string str(text, text_len);
		Data->latest_version_num = atoi(str.c_str());
	} else if (g_str_has_prefix(element, "version_msg_title")) {
		const char *locale = element + (sizeof("version_msg_title")-1);
		if (locale[0] == '\0') {
			if (Data->version_msg_title.empty()) {
				Data->version_msg_title.assign(text, text_len);
			}
		} else if (Data->locale_name == locale+1) {
			Data->version_msg_title.assign(text, text_len);
		}
	} else if (g_str_has_prefix(element, "version_msg_content")) {
		const char *locale = element + (sizeof("version_msg_content")-1);
		if (locale[0] == '\0') {
			if (Data->version_msg_content.empty()) {
				Data->version_msg_content.assign(text, text_len);
			}
		} else if (Data->locale_name == locale+1) {
			Data->version_msg_content.assign(text, text_len);
		}
	} else if (g_str_has_prefix(element, "latest_news")) {
		const char *locale = element + (sizeof("latest_news")-1);
		if (locale[0] == '\0') {
			if (Data->latest_news.empty()) {
				Data->latest_news.assign(text, text_len);
			}
		} else if (Data->locale_name == locale+1) {
			Data->latest_news.assign(text, text_len);
		}
	} else if (g_str_has_prefix(element, "links")) {
		const char *locale = element + (sizeof("links")-1);
		if (locale[0] == '\0') {
			if (Data->links.empty()) {
				Data->links.assign(text, text_len);
			}
		} else if (Data->locale_name == locale+1) {
			Data->links.assign(text, text_len);
		}
	}
}

static void on_get_http_response(const char *buffer, size_t buffer_len, gpointer userdata)
{
	if (!buffer)
		return;
	const char *p = g_strstr_len(buffer, buffer_len, "\r\n\r\n");
	if (!p) {
		return;
	}
	p += 4;
	if(g_str_has_prefix(p, UTF8_BOM))
		p += (sizeof(UTF8_BOM)-1); // better than strlen(UTF8_BOM);
	updateinfo_ParseUserData Data;
	Data.latest_version_num = 0;
	const gchar* const *languages = g_get_language_names();
	const char *locale = languages[0];
	if (locale && locale[0] != '\0') {
		const char *p = strchr(locale, '.');
		if (p) {
			Data.locale_name.assign(locale, p - locale);
		} else {
			Data.locale_name = locale;
		}
	}
	GMarkupParser parser;
	parser.start_element = NULL;
	parser.end_element = NULL;
	parser.text = updateinfo_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, p, buffer_len - (p - buffer), NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);

	bool updated = false;
	if (Data.latest_version_num != latest_version_num) {
		updated = true;
		latest_version_num = Data.latest_version_num;
		version_msg_title = Data.version_msg_title;
		version_msg_content = Data.version_msg_content;
	}
	if (Data.latest_version_num > my_version_num && Data.latest_version_num != last_prompt_num) {
		std::string content = version_msg_content;
		content += "\n\n";
		content += _("Visit StarDict website now?");
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(plugin_info->mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_YES_NO, "%s", content.c_str());
		GtkWidget *prompt = gtk_check_button_new_with_mnemonic(_("_Don't show this until the next update."));
		gtk_widget_show(prompt);
		gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG(dialog))), prompt);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
		gtk_window_set_title (GTK_WINDOW (dialog), version_msg_title.c_str());
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES) {
			plugin_service->show_url("http://stardict-4.sourceforge.net");
		}
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prompt))) {
			updated = true;
			last_prompt_num = Data.latest_version_num;
		}
		gtk_widget_destroy (dialog);
	}
	if (Data.latest_news != latest_news) {
		updated = true;
		latest_news = Data.latest_news;
	}
	if (updated) {
		GKeyFile *keyfile = g_key_file_new();
		g_key_file_set_string(keyfile, "update", "version_msg_title", version_msg_title.c_str());
		g_key_file_set_string(keyfile, "update", "version_msg_content", version_msg_content.c_str());
		g_key_file_set_string(keyfile, "update", "latest_news", latest_news.c_str());
		g_key_file_set_integer(keyfile, "update", "latest_version_num", latest_version_num);
		g_key_file_set_integer(keyfile, "update", "last_prompt_num", last_prompt_num);
		g_key_file_set_boolean(keyfile, "misc", "show_ads", show_ads);
		gsize length;
		gchar *content = g_key_file_to_data(keyfile, &length, NULL);
		std::string res = get_cfg_filename();
		g_file_set_contents(res.c_str(), content, length, NULL);
		g_free(content);
	}
	if (show_ads) {
		plugin_service->set_news(latest_news.c_str(), Data.links.c_str());
	}
}

// Don't use g_idle_add to call send_http_request(), as it may be called before mainloop, and before the window is created, which may cause crash when set the news.
DLLIMPORT void stardict_misc_plugin_on_mainwin_finish(void)
{
	plugin_service->send_http_request("stardict-4.sourceforge.net", "/UPDATE", on_get_http_response, NULL);
}

DLLIMPORT bool stardict_misc_plugin_init(void)
{
	std::string res = get_cfg_filename();
	if (!g_file_test(res.c_str(), G_FILE_TEST_EXISTS)) {
		g_file_set_contents(res.c_str(), "[update]\nlatest_version_num=0\nlast_prompt_num=0\nversion_msg_title=\nversion_msg_content=\nlatest_news=\n[misc]\nshow_ads=true\n", -1, NULL);
	}
	GKeyFile *keyfile = g_key_file_new();
	g_key_file_load_from_file(keyfile, res.c_str(), G_KEY_FILE_NONE, NULL);
	GError *err;
	err = NULL;
	latest_version_num = g_key_file_get_integer(keyfile, "update", "latest_version_num", &err);
	if (err) {
		g_error_free (err);
		latest_version_num = 0;
	}
	err = NULL;
	last_prompt_num = g_key_file_get_integer(keyfile, "update", "last_prompt_num", &err);
	if (err) {
		g_error_free (err);
		last_prompt_num = 0;
	}
	char *str;
	str = g_key_file_get_string(keyfile, "update", "version_msg_title", NULL);
	if (str) {
		version_msg_title = str;
		g_free(str);
	}
	str = g_key_file_get_string(keyfile, "update", "version_msg_content", NULL);
	if (str) {
		version_msg_content = str;
		g_free(str);
	}
	str = g_key_file_get_string(keyfile, "update", "latest_news", NULL);
	if (str) {
		latest_news = str;
		g_free(str);
	}
	err = NULL;
	show_ads = g_key_file_get_boolean(keyfile, "misc", "show_ads", &err);
	if (err) {
		g_error_free (err);
		show_ads = true;
	}
	g_key_file_free(keyfile);
	g_print(_("Update info plug-in loaded.\n"));
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
