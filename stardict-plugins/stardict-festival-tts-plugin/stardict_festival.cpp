#include "stardict_festival.h"
#ifdef HAVE_CONFIG_H
	#undef HAVE_CONFIG_H
	#include <festival/festival.h>
	#define HAVE_CONFIG_H
#endif
#include <glib/gi18n.h>

static void saytext(const char *text)
{
	festival_say_text(text);
}

bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Festival plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_TTS;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("Festival"), _("Festival TTS."), _("Pronounce words by Festival TTS engine."));
	return false;
}

void stardict_plugin_exit(void)
{
}

bool stardict_tts_plugin_init(StarDictTtsPlugInObject *obj)
{
	festival_initialize(1, 210000);
	obj->saytext_func = saytext;
	g_print(_("Festival plug-in loaded.\n"));
	return false;
}
