#include "stardict_spell.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <enchant.h>
#include <string>

static EnchantBroker *broker = NULL;
static EnchantDict *dict = NULL;

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

static void lookup(const char *word, char **return_word, char **return_data)
{
	int return_val = enchant_dict_check(dict, word, -1);
	char **suggestion;
	if (return_val <= 0 || ((suggestion = enchant_dict_suggest(dict, word, -1, NULL))==NULL)) {
		*return_word = NULL;
		return;
	}
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
	*return_word = g_strdup(word);
	*return_data = build_dictdata('x', definition.c_str());
}


bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Aspell plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_VIRTUALDICT;

	return false;
}

void stardict_plugin_exit(void)
{
	if (broker) {
		if (dict)
			enchant_broker_free_dict(broker, dict);
		enchant_broker_free(broker);
	}
}

bool stardict_virtualdict_plugin_init(StarDictVirtualDictPlugInObject *obj)
{
	obj->lookup_func = lookup;
	obj->is_instant = true;
	obj->dict_name = _("Spell suggestion");
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
	g_print("Spell plug-in loaded.\n");
	return false;
}
