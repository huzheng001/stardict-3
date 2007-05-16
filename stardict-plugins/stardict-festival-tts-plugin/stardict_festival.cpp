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
