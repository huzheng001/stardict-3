#ifndef _STARDICT_SPECIALDICT_PLUGIN_H_
#define _STARDICT_SPECIALDICT_PLUGIN_H_

#include <gtk/gtk.h>

struct StarDictSpecialDictPlugInObject{
	StarDictSpecialDictPlugInObject();

	typedef void (*render_widget_func_t)(bool ismainwin, size_t dictid, const gchar *orig_word, gchar **Word, gchar ***WordData, GtkWidget **widget);
	render_widget_func_t render_widget_func;
	const char *dict_type;
};

#endif
