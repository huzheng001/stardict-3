#include "netdictplugin.h"
#include <glib.h>

NetDictResponse::~NetDictResponse()
{
	g_free(word);
	g_free(data);
}

StarDictNetDictPlugInObject::StarDictNetDictPlugInObject()
{
	lookup_func = 0;
}
