#include "stardict_festival.h"
#include <festival/festival.h>
#include <glib/gi18n.h>

static const StarDictPluginSystemInfo *plugin_info = NULL;
static std::string voice_engine;

static std::string get_cfg_filename()
{
#ifdef _WIN32
	std::string res = g_get_user_config_dir();
	res += G_DIR_SEPARATOR_S "StarDict" G_DIR_SEPARATOR_S "festival.cfg";
#else
	std::string res;
	gchar *tmp = g_build_filename(g_get_home_dir(), ".stardict", NULL);
	res=tmp;
	g_free(tmp);
	res += G_DIR_SEPARATOR_S "festival.cfg";
#endif
	return res;
}

static void saytext(const char *text)
{
	festival_say_text(text);
}

static void configure()
{
	GtkWidget *window = gtk_dialog_new_with_buttons(_("Festival TTS configuration"), GTK_WINDOW(plugin_info->pluginwin), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *hbox = gtk_hbox_new(false, 5);
	GtkWidget *label = gtk_label_new(_("Voice type:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	GtkWidget *combobox = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), _("Default"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Male1-kal");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Male2-ked");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Male3-jmk");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Male4-bdl");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Male5-awb");
	gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), "Female1-slt");
	gint old_index;
	if (voice_engine == "voice_kal_diphone")
		old_index = 1;
	else if (voice_engine == "voice_ked_diphone")
		old_index = 2;
	else if (voice_engine == "voice_cmu_us_jmk_arctic_hts")
		old_index = 3;
	else if (voice_engine == "voice_cmu_us_bdl_arctic_hts")
		old_index = 4;
	else if (voice_engine == "voice_cmu_us_awb_arctic_hts")
		old_index = 5;
	else if (voice_engine == "voice_cmu_us_slt_arctic_hts")
		old_index = 6;
	else
		old_index = 0;
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), old_index);
	gtk_box_pack_start(GTK_BOX(hbox), combobox, false, false, 0);
	gtk_widget_show_all(hbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(window)->vbox), hbox);
	gtk_dialog_run(GTK_DIALOG(window));
	gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
	if (index != old_index) {
		if (index == 1)
			voice_engine = "voice_kal_diphone";
		else if (index == 2)
			voice_engine = "voice_ked_diphone";
		else if (index == 3)
			voice_engine = "voice_cmu_us_jmk_arctic_hts";
		else if (index == 4)
			voice_engine = "voice_cmu_us_bdl_arctic_hts";
		else if (index == 5)
			voice_engine = "voice_cmu_us_awb_arctic_hts";
		else if (index == 6)
			voice_engine = "voice_cmu_us_slt_arctic_hts";
		else
			voice_engine.clear();
		if (!voice_engine.empty()) {
			std::string command = "(";
			command += voice_engine;
			command += ")";
			festival_eval_command(command.c_str());
		}
		gchar *data = g_strdup_printf("[festival]\nvoice=%s\n", voice_engine.c_str());
		std::string res = get_cfg_filename();
		g_file_set_contents(res.c_str(), data, -1, NULL);
		g_free(data);
	}
	gtk_widget_destroy (window);
}

bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Festival plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_TTS;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("Festival"), _("Festival TTS."), _("Pronounce words by Festival TTS engine."));
	obj->configure_func = configure;
	plugin_info = obj->plugin_info;
	return false;
}

void stardict_plugin_exit(void)
{
}

bool stardict_tts_plugin_init(StarDictTtsPlugInObject *obj)
{
	festival_initialize(1, 210000);
	std::string res = get_cfg_filename();
	if (!g_file_test(res.c_str(), G_FILE_TEST_EXISTS)) {
		g_file_set_contents(res.c_str(), "[festival]\nvoice=\n", -1, NULL);
	}
	GKeyFile *keyfile = g_key_file_new();
	g_key_file_load_from_file(keyfile, res.c_str(), G_KEY_FILE_NONE, NULL);
	gchar *str = g_key_file_get_string(keyfile, "festival", "voice", NULL);
	g_key_file_free(keyfile);
	if (str) {
		voice_engine = str;
		g_free(str);
	}
	if (!voice_engine.empty()) {
		std::string command = "(";
		command += voice_engine;
		command += ")";
		festival_eval_command(command.c_str());
	}
	obj->saytext_func = saytext;
	obj->tts_name = _("Festival TTS");
	g_print(_("Festival plug-in loaded.\n"));
	return false;
}
