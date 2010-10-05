#ifndef _COLLATION_H_
#define _COLLATION_H_

typedef enum {
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
extern void utf8_collate_end();


#endif
