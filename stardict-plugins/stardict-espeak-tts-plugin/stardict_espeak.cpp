#include "stardict_espeak.h"
#include <espeak/speak_lib.h>
#include <glib/gi18n.h>

static const StarDictPluginSystemInfo *plugin_info = NULL;
static std::string voice_engine;

static std::string get_cfg_filename()
{
#ifdef _WIN32
	std::string res = g_get_user_config_dir();
	res += G_DIR_SEPARATOR_S "StarDict" G_DIR_SEPARATOR_S "espeak.cfg";
#else
	std::string res;
	gchar *tmp = g_build_filename(g_get_home_dir(), ".stardict", NULL);
	res=tmp;
	g_free(tmp);
	res += G_DIR_SEPARATOR_S "espeak.cfg";
#endif
	return res;
}

static void saytext(const char *text)
{
	espeak_Synth(text, strlen(text)+1, 0, POS_CHARACTER, 0, espeakCHARS_UTF8, NULL, NULL);
}

static void configure()
{
	GtkWidget *window = gtk_dialog_new_with_buttons(_("Espeak TTS configuration"), GTK_WINDOW(plugin_info->pluginwin), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *hbox = gtk_hbox_new(false, 5);
	GtkWidget *label = gtk_label_new(_("Voice type:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	GtkWidget *combobox = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), _("Default"));
	gint old_index = 0;
	const espeak_VOICE **voices = espeak_ListVoices(NULL);
	size_t i = 0;
	while (voices[i]) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), voices[i]->name);
		if (old_index == 0 && voice_engine == voices[i]->name) {
			old_index = i + 1;
		}
		i++;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), old_index);
	gtk_box_pack_start(GTK_BOX(hbox), combobox, false, false, 0);
	gtk_widget_show_all(hbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(window)->vbox), hbox);
	gtk_dialog_run(GTK_DIALOG(window));
	gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
	if (index != old_index) {
		if (index == 0) {
			voice_engine.clear();
		} else {
			voice_engine = voices[index -1]->name;
		}
		if (voice_engine.empty()) {
			espeak_SetVoiceByName("english");
		} else {
			espeak_SetVoiceByName(voice_engine.c_str());
		}
		gchar *data = g_strdup_printf("[espeak]\nvoice=%s\n", voice_engine.c_str());
		std::string res = get_cfg_filename();
		g_file_set_contents(res.c_str(), data, -1, NULL);
		g_free(data);
	}
	gtk_widget_destroy (window);
}

bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Espeak plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_TTS;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("Espeak"), _("Espeak TTS."), _("Pronounce words by Espeak TTS engine."));
	obj->configure_func = configure;
	plugin_info = obj->plugin_info;
	return false;
}

void stardict_plugin_exit(void)
{
	espeak_Terminate();
}

bool stardict_tts_plugin_init(StarDictTtsPlugInObject *obj)
{
	espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 0, NULL, 0);
	std::string res = get_cfg_filename();
	if (!g_file_test(res.c_str(), G_FILE_TEST_EXISTS)) {
		g_file_set_contents(res.c_str(), "[espeak]\nvoice=\n", -1, NULL);
	}
	GKeyFile *keyfile = g_key_file_new();
	g_key_file_load_from_file(keyfile, res.c_str(), G_KEY_FILE_NONE, NULL);
	gchar *str = g_key_file_get_string(keyfile, "espeak", "voice", NULL);
	g_key_file_free(keyfile);
	if (str) {
		voice_engine = str;
		g_free(str);
	}
	if (!voice_engine.empty()) {
		espeak_SetVoiceByName(voice_engine.c_str());
	}
	obj->saytext_func = saytext;
	obj->tts_name = _("Espeak TTS");
	g_print(_("Espeak plug-in loaded.\n"));
	return false;
}
