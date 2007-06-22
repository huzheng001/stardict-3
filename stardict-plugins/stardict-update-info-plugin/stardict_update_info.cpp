#include "stardict_update_info.h"
#include <glib/gi18n.h>

const int my_version_num = 30000000; // As 3,00,00,000, so the version is 3.0.0.0

static int lastest_version_num;

static std::string get_cfg_filename()
{
#ifdef _WIN32
	std::string res = g_get_user_config_dir();
	res += G_DIR_SEPARATOR_S "StarDict" G_DIR_SEPARATOR_S "update_info.cfg";
#else
	std::string res;
	gchar *tmp = g_build_filename(g_get_home_dir(), ".stardict", NULL);
	res=tmp;
	g_free(tmp);
	res += G_DIR_SEPARATOR_S "update_info.cfg";
#endif
	return res;
}

static void configure()
{
	GtkWidget *window = gtk_dialog_new_with_buttons(_("Update information"), NULL, GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *hbox = gtk_hbox_new(false, 5);
	GtkWidget *label = gtk_label_new(_("There have no update presently. The current version is the newest."));
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	gtk_widget_show_all(hbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(window)->vbox), hbox);
	gtk_dialog_run(GTK_DIALOG(window));
	gtk_widget_destroy (window);
}

bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Update info plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_MISC;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("Update Info"), _("Update information."), _("Get the update information from the Internet."));
	obj->configure_func = configure;
	return false;
}

void stardict_plugin_exit(void)
{
}

bool stardict_misc_plugin_init(void)
{
	std::string res = get_cfg_filename();
	if (!g_file_test(res.c_str(), G_FILE_TEST_EXISTS)) {
		g_file_set_contents(res.c_str(), "[update]\nlastest_version_num=0\n", -1, NULL);
	}
	GKeyFile *keyfile = g_key_file_new();
	g_key_file_load_from_file(keyfile, res.c_str(), G_KEY_FILE_NONE, NULL);
	GError *err;
	err = NULL;
	lastest_version_num = g_key_file_get_integer(keyfile, "update", "lastest_version_num", &err);
	if (err) {
		g_error_free (err);
		lastest_version_num = 0;
	}
	g_key_file_free(keyfile);
	g_print(_("Update info plug-in loaded.\n"));
	return false;
}
