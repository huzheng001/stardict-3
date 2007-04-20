#include "advertisement.h"
#include <glib.h>

static const StarDictVirtualDictPlugInSlots *vd_slots = NULL;

static void lookup(char *word)
{
}

bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Advertisement plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_VIRTUALDICT;
	vd_slots = obj->vd_slots;

	return false;
}

void stardict_plugin_exit(void)
{
}

bool stardict_virtualdict_plugin_init(StarDictVirtualDictPlugInObject *obj)
{
	obj->lookup_func = lookup;
	g_print("Hello!\n");
	return false;
}
