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

#ifndef _COLLATION_H_
#define _COLLATION_H_

typedef enum {
	COLLATE_FUNC_NONE = -1,
	UTF8_GENERAL_CI = 0,
	UTF8_UNICODE_CI,
	UTF8_BIN,
	UTF8_CZECH_CI,
	UTF8_DANISH_CI,
	UTF8_ESPERANTO_CI,
	UTF8_ESTONIAN_CI,
	UTF8_HUNGARIAN_CI,
	UTF8_ICELANDIC_CI,
	UTF8_LATVIAN_CI,
	UTF8_LITHUANIAN_CI,
	UTF8_PERSIAN_CI,
	UTF8_POLISH_CI,
	UTF8_ROMAN_CI,
	UTF8_ROMANIAN_CI,
	UTF8_SLOVAK_CI,
	UTF8_SLOVENIAN_CI,
	UTF8_SPANISH_CI,
	UTF8_SPANISH2_CI,
	UTF8_SWEDISH_CI,
	UTF8_TURKISH_CI,
	COLLATE_FUNC_NUMS
} CollateFunctions;

extern int utf8_collate_init(CollateFunctions func);
extern int utf8_collate_init_all();
extern int utf8_collate(const char *str1, const char *str2, CollateFunctions func);
extern void utf8_collate_end(CollateFunctions func);
extern void utf8_collate_end_all();
extern CollateFunctions int_to_colate_func(int func);

#endif
