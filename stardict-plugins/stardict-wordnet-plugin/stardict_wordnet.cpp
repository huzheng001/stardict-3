#include "stardict_wordnet.h"
#include "court_widget.h"
#include <glib/gi18n.h>

#ifdef _WIN32
#include <windows.h>
#endif

static const StarDictPluginSystemInfo *plugin_info = NULL;
static const StarDictPluginSystemService *plugin_service;
static gint widget_width, widget_height;
static bool text_or_graphic_mode;

static std::string get_cfg_filename()
{
#ifdef _WIN32
	std::string res = g_get_user_config_dir();
	res += G_DIR_SEPARATOR_S "StarDict" G_DIR_SEPARATOR_S "wordnet.cfg";
#else
	std::string res;
	gchar *tmp = g_build_filename(g_get_home_dir(), ".stardict", NULL);
	res=tmp;
	g_free(tmp);
	res += G_DIR_SEPARATOR_S "wordnet.cfg";
#endif
	return res;
}

static void render_widget(bool ismainwin, size_t dictid, const gchar *orig_word, gchar **Word, gchar ***WordData, GtkWidget **widget)
{
	if (!ismainwin)
		return;
	if (text_or_graphic_mode)
		return;

	WnCourt *wncourt = new WnCourt(dictid, plugin_service->lookup_dict, plugin_service->FreeResultData, plugin_service->ShowPangoTips, &widget_width, &widget_height);
	wncourt->set_word(orig_word, Word, WordData);
	*widget = wncourt->get_widget();
}

static void save_conf_file()
{
	const char *tmp;
	if (text_or_graphic_mode) {
		tmp = "true";
	} else {
		tmp = "false";
	}
	gchar *data = g_strdup_printf("[wordnet]\ntext_or_graphic_mode=%s\nwidth=%d\nheight=%d\n", tmp, widget_width, widget_height);
	std::string res = get_cfg_filename();
	g_file_set_contents(res.c_str(), data, -1, NULL);
	g_free(data);
}

static void configure()
{
	GtkWidget *window = gtk_dialog_new_with_buttons(_("WordNet configuration"), GTK_WINDOW(plugin_info->pluginwin), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *vbox = gtk_vbox_new(false, 5);
	GtkWidget *graphic_button = gtk_radio_button_new_with_label(NULL, _("Graphic mode."));
	gtk_box_pack_start(GTK_BOX(vbox), graphic_button, false, false, 0);
	GtkWidget *text_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(graphic_button), _("Text mode."));
	gtk_box_pack_start(GTK_BOX(vbox), text_button, false, false, 0);
	if (text_or_graphic_mode) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(text_button), true);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(graphic_button), true);
	}
	gtk_widget_show_all(vbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(window)->vbox), vbox);
	gtk_dialog_run(GTK_DIALOG(window));
	gboolean new_text_or_graphic_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(text_button));
	if (new_text_or_graphic_mode != text_or_graphic_mode) {
		text_or_graphic_mode = new_text_or_graphic_mode;
		save_conf_file();
	}
	gtk_widget_destroy (window);
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: WordNet dict rendering plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_SPECIALDICT;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("WordNet dict rendering"), _("WordNet dict rendering engine."), _("Render the WordNet dictionary.\nStatement: The engine of this plugin comes from dedict(http://sevenpie.net), which is developed by Bian Peng &lt;tianpmoon@gmail.com&gt;, much thanks for his open source sharing!"));
	obj->configure_func = configure;
	plugin_info = obj->plugin_info;
	plugin_service = obj->plugin_service;
	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
	save_conf_file();
}

DLLIMPORT bool stardict_specialdict_plugin_init(StarDictSpecialDictPlugInObject *obj)
{
	std::string res = get_cfg_filename();
	if (!g_file_test(res.c_str(), G_FILE_TEST_EXISTS)) {
		g_file_set_contents(res.c_str(), "[wordnet]\ntext_or_graphic_mode=false\nwidth=400\nheight=300\n", -1, NULL);
	}
	GKeyFile *keyfile = g_key_file_new();
	g_key_file_load_from_file(keyfile, res.c_str(), G_KEY_FILE_NONE, NULL);
	GError *err;
	err = NULL;
	text_or_graphic_mode = g_key_file_get_boolean(keyfile, "wordnet", "text_or_graphic_mode", &err);
	if (err) {
		g_error_free (err);
		text_or_graphic_mode = false;
	}
	err = NULL;
	widget_width = g_key_file_get_integer(keyfile, "wordnet", "width", &err);
	if (err) {
		g_error_free (err);
		widget_width = 400;
	}
	err = NULL;
	widget_height = g_key_file_get_integer(keyfile, "wordnet", "height", &err);
	if (err) {
		g_error_free (err);
		widget_height = 300;
	}
	g_key_file_free(keyfile);
	obj->render_widget_func = render_widget;
	obj->dict_type = "wordnet";
	g_print(_("WordNet dict rendering plug-in loaded.\n"));
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
