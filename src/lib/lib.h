#ifndef __SD_LIB_H__
#define __SD_LIB_H__

#include "stardict_libconfig.h"

#include "common.hpp"
#include "data.hpp"
#include "stddict.hpp"

#ifdef SD_CLIENT_CODE
#include "treedict.hpp"
#endif

static inline gchar* stardict_datadup(gconstpointer mem)
{
        return (gchar *)g_memdup(mem, sizeof(guint32) + *reinterpret_cast<const guint32 *>(mem));
}

typedef enum {
	qtSIMPLE, qtPATTERN, qtFUZZY, qtREGEX, qtDATA
} query_t;
	
extern query_t analyse_query(const char *s, std::string& res); 
extern void stardict_input_escape(const char *text, std::string &res);

#endif//!__SD_LIB_H__
