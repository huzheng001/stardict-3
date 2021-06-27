/*
 * Copyright 2011 kubtek <kubtek@mail.com>
 *
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <glib.h>

#include <string>
#include "utils.h"
#include "m_ctype.h"
#include "collation.h"

using namespace stardict_collation;

static CHARSET_INFO *get_cs(CollateFunctions func)
{
	CHARSET_INFO *cs;
	if (func == UTF8_GENERAL_CI)
		cs = &my_charset_utf8_general_ci;
	else if (func == UTF8_BIN)
		cs = &my_charset_utf8_bin;
	else if (func == UTF8_UNICODE_CI)
		cs = &my_charset_utf8_general_uca_ci;
	else if (func == UTF8_ICELANDIC_CI)
		cs = &my_charset_utf8_icelandic_uca_ci;
	else if (func == UTF8_LATVIAN_CI)
		cs = &my_charset_utf8_latvian_uca_ci;
	else if (func == UTF8_ROMANIAN_CI)
		cs = &my_charset_utf8_romanian_uca_ci;
	else if (func == UTF8_SLOVENIAN_CI)
		cs = &my_charset_utf8_slovenian_uca_ci;
	else if (func == UTF8_POLISH_CI)
		cs = &my_charset_utf8_polish_uca_ci;
	else if (func == UTF8_ESTONIAN_CI)
		cs = &my_charset_utf8_estonian_uca_ci;
	else if (func == UTF8_SPANISH_CI)
		cs = &my_charset_utf8_spanish_uca_ci;
	else if (func == UTF8_SWEDISH_CI)
		cs = &my_charset_utf8_swedish_uca_ci;
	else if (func == UTF8_TURKISH_CI)
		cs = &my_charset_utf8_turkish_uca_ci;
	else if (func == UTF8_CZECH_CI)
		cs = &my_charset_utf8_czech_uca_ci;
	else if (func == UTF8_DANISH_CI)
		cs = &my_charset_utf8_danish_uca_ci;
	else if (func == UTF8_LITHUANIAN_CI)
		cs = &my_charset_utf8_lithuanian_uca_ci;
	else if (func == UTF8_SLOVAK_CI)
		cs = &my_charset_utf8_slovak_uca_ci;
	else if (func == UTF8_SPANISH2_CI)
		cs = &my_charset_utf8_spanish2_uca_ci;
	else if (func == UTF8_ROMAN_CI)
		cs = &my_charset_utf8_roman_uca_ci;
	else if (func == UTF8_PERSIAN_CI)
		cs = &my_charset_utf8_persian_uca_ci;
	else if (func == UTF8_ESPERANTO_CI)
		cs = &my_charset_utf8_esperanto_uca_ci;
	else if (func == UTF8_HUNGARIAN_CI)
		cs = &my_charset_utf8_hungarian_uca_ci;
	else
		cs = NULL;
	return cs;
}

/* This template class is instantiated once for each item of CollateFunctions enumeration,
 * excluding COLLATE_FUNC_NUMS.
 * Each instance has a separate static field root_block,
 * independent static functions alloc and free_all.
 * For example, CollationAllocator<UTF8_GENERAL_CI>::alloc() saves allocated blocks
 * in CollationAllocator<UTF8_GENERAL_CI>::root_block list.
 * */
template<CollateFunctions f>
class CollationAllocator
{
private:
	static GSList *root_block;
public:
	static void* alloc(uint size)
	{
		void *mem = g_malloc(size);
		root_block = g_slist_prepend (root_block, mem);
		return mem;
	}
	static void free_all()
	{
		GSList *list = root_block;
		while (list) {
			g_free(list->data);
			list = list->next;
		}
		g_slist_free(root_block);
		root_block = NULL;
	}
};

template<CollateFunctions f>
GSList* CollationAllocator<f>::root_block = NULL;

struct CollationData
{
	void *(*alloc)(uint);
	void (*free_all)();
	/* reference count. How many times this collation was initialized. */
	int ref;
};

CollationData coll_data[] = {
	{ CollationAllocator<UTF8_GENERAL_CI>::alloc, CollationAllocator<UTF8_GENERAL_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_UNICODE_CI>::alloc, CollationAllocator<UTF8_UNICODE_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_BIN>::alloc, CollationAllocator<UTF8_BIN>::free_all, 0 },
	{ CollationAllocator<UTF8_CZECH_CI>::alloc, CollationAllocator<UTF8_CZECH_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_DANISH_CI>::alloc, CollationAllocator<UTF8_DANISH_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_ESPERANTO_CI>::alloc, CollationAllocator<UTF8_ESPERANTO_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_ESTONIAN_CI>::alloc, CollationAllocator<UTF8_ESTONIAN_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_HUNGARIAN_CI>::alloc, CollationAllocator<UTF8_HUNGARIAN_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_ICELANDIC_CI>::alloc, CollationAllocator<UTF8_ICELANDIC_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_LATVIAN_CI>::alloc, CollationAllocator<UTF8_LATVIAN_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_LITHUANIAN_CI>::alloc, CollationAllocator<UTF8_LITHUANIAN_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_PERSIAN_CI>::alloc, CollationAllocator<UTF8_PERSIAN_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_POLISH_CI>::alloc, CollationAllocator<UTF8_POLISH_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_ROMAN_CI>::alloc, CollationAllocator<UTF8_ROMAN_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_ROMANIAN_CI>::alloc, CollationAllocator<UTF8_ROMANIAN_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_SLOVAK_CI>::alloc, CollationAllocator<UTF8_SLOVAK_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_SLOVENIAN_CI>::alloc, CollationAllocator<UTF8_SLOVENIAN_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_SPANISH_CI>::alloc, CollationAllocator<UTF8_SPANISH_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_SPANISH2_CI>::alloc, CollationAllocator<UTF8_SPANISH2_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_SWEDISH_CI>::alloc, CollationAllocator<UTF8_SWEDISH_CI>::free_all, 0 },
	{ CollationAllocator<UTF8_TURKISH_CI>::alloc, CollationAllocator<UTF8_TURKISH_CI>::free_all, 0 }
};

int utf8_collate_init(CollateFunctions func)
{
	g_assert(0<=func && func<COLLATE_FUNC_NUMS);
	if(coll_data[func].ref++ > 0)
		return FALSE;
	CHARSET_INFO *cs = get_cs(func);
	if (cs) {
		if ((cs->cset->init && cs->cset->init(cs, coll_data[func].alloc)) ||
			(cs->coll->init && cs->coll->init(cs, coll_data[func].alloc)))
			return TRUE;
		else
			return FALSE;
	} else
		return TRUE;
}

int utf8_collate_init_all()
{
	CHARSET_INFO *cs;
	for (int func=0; func<COLLATE_FUNC_NUMS; func++) {
		if(coll_data[func].ref++ > 0)
			continue;
		cs = get_cs((CollateFunctions)func);
		if (cs) {
			if ((cs->cset->init && cs->cset->init(cs, coll_data[func].alloc)) ||
				(cs->coll->init && cs->coll->init(cs, coll_data[func].alloc)))
				return TRUE;
		} else
			return TRUE;
	}
	return FALSE;
}

int utf8_collate(const char *str1, const char *str2, CollateFunctions func)
{
	CHARSET_INFO *cs = get_cs(func);
	if (cs) {
		return cs->coll->strnncoll(cs, (const uchar*)str1, strlen(str1), (const uchar*)str2, strlen(str2), 0);
	} else
		return 0; //Should never happen.
}

void utf8_collate_end(CollateFunctions func)
{
	g_assert(0<=func && func<COLLATE_FUNC_NUMS);
	if(coll_data[func].ref <= 0) {
		coll_data[func].ref = 0;
		return;
	}
	if(--coll_data[func].ref == 0)
		coll_data[func].free_all();
}

void utf8_collate_end_all()
{
	for (int func=0; func<COLLATE_FUNC_NUMS; func++) {
		if(coll_data[func].ref <= 0) {
			coll_data[func].ref = 0;
			continue;
		}
		if(--coll_data[func].ref == 0)
			coll_data[func].free_all();
	}
}

CollateFunctions int_to_colate_func(int func)
{
	if(0 <= func && func < COLLATE_FUNC_NUMS)
		return CollateFunctions(func);
	return COLLATE_FUNC_NONE;
}
