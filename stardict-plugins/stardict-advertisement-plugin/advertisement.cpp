#include "advertisement.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <string>
#include <map>
#include <list>
#include <vector>


static const StarDictPluginInfo *plugin_info = NULL;

struct DictEntry {
	std::string word;
	char *data;
};

static std::multimap<std::string, DictEntry> dict_map;
static std::list<char *> dictdata_list;

static void my_strstrip(gchar *str)
{
	char *p1, *p2;
	p1=str;
	p2=str;
	while (*p1 != '\0') {
		if (*p1 == '\\') {
			p1++;
			if (*p1 == 'n') {
				*p2='\n';
				p2++;
				p1++;
				continue;
			} else if (*p1 == '\\') {
				*p2='\\';
				p2++;
				p1++;
				continue;
			} else {
				*p2='\\';
				p2++;
				*p2=*p1;
				p2++;
				p1++;
				continue;
			}
		} else {
			*p2 = *p1;
			p2++;
			p1++;
			continue;
		}
	}
	*p2 = '\0';
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

static bool load_dict(const char *filename)
{
	gchar *contents;
	gboolean success = g_file_get_contents(filename, &contents, NULL, NULL);
	if (!success) {
		g_print("File %s doesn't exist!\n", filename);
		return true;
	}
	gchar *p, *p1, *p2;
	p = contents;
	int step = 0;
	std::list<std::string> wordlist;
	DictEntry dictentry;
	char dict_type = 'm';
	while (true) {
		p1 = strchr(p, '\n');
		if (!p1)
			break;
		*p1 = '\0';
		if (step == 0) { // Word list.
			if (*p == '\0')
				break;
			wordlist.clear();
			while (true) {
				p2 = strchr(p, '\t');
				if (!p2)
					break;
				*p2 = '\0';
				wordlist.push_back(p);
				p = p2 +1;
			}
			wordlist.push_back(p);
			step = 1;
		} else if (step == 1) { // Type
			dict_type = *p;
			step = 2;
		} else if (step == 2) { // Definition
			my_strstrip(p);
			dictentry.data = build_dictdata(dict_type, p);
			dictdata_list.push_back(dictentry.data);
			step = 3;
		} else { // New line.
			for (std::list<std::string>::iterator i = wordlist.begin(); i != wordlist.end(); ++i) {
				dictentry.word = *i;
				gchar *lower_str = g_utf8_strdown(dictentry.word.c_str(), dictentry.word.length());
				dict_map.insert(std::pair<std::string, DictEntry>(lower_str, dictentry));
				g_free(lower_str);
			}
			step = 0;
		}
		p = p1 + 1;
	}
	g_free(contents);
	return false;
}

static void unload_dict()
{
	for (std::list<char *>::iterator i = dictdata_list.begin(); i != dictdata_list.end(); ++i) {
		g_free(*i);
	}
}

static void lookup(const char *word, char ***pppWord, char ****ppppWordData)
{
	gchar *lower_str = g_utf8_strdown(word, -1);
	std::multimap<std::string, DictEntry>::iterator iter = dict_map.find(lower_str);
	if (iter == dict_map.end()) {
		*pppWord = NULL;
	} else {
		std::vector< std::pair<char *, char *> > result;
		char *return_word, *return_data, *mem;
		do {
			return_word = g_strdup(iter->second.word.c_str());
			mem = iter->second.data;
			return_data = (char *)g_memdup(mem, sizeof(guint32) + *reinterpret_cast<const guint32 *>(mem));
			result.push_back(std::pair<char *, char *>(return_word, return_data));
			iter++;
		} while (iter != dict_map.upper_bound(lower_str));
		*pppWord = (gchar **)g_malloc(sizeof(gchar *)*(result.size()+1));
		*ppppWordData = (gchar ***)g_malloc(sizeof(gchar **)*result.size());
		for (std::vector< std::pair<char *, char *> >::size_type i = 0; i< result.size(); i++) {
			(*pppWord)[i] = result[i].first;
			(*ppppWordData)[i] = (gchar **)g_malloc(sizeof(gchar *)*2);
			(*ppppWordData)[i][0] = result[i].second;
			(*ppppWordData)[i][1] = NULL;
		}
		(*pppWord)[result.size()] = NULL;
	}
	g_free(lower_str);
}

bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Advertisement plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_VIRTUALDICT;
	plugin_info = obj->plugin_info;

	return false;
}

void stardict_plugin_exit(void)
{
	unload_dict();
}

bool stardict_virtualdict_plugin_init(StarDictVirtualDictPlugInObject *obj)
{
	obj->lookup_func = lookup;
	obj->is_instant = true;
	obj->dict_name = _("Advertisement");
	std::string filename = plugin_info->datadir;
	filename += G_DIR_SEPARATOR_S "data" G_DIR_SEPARATOR_S "advertisement.txt";
	bool failed = load_dict(filename.c_str());
	if (failed)
		return true;
	g_print("Advertisement plug-in loaded.\n");
	return false;
}
