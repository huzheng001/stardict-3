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

#include "stardict_gucharmap.h"
#include <glib/gi18n.h>
#include <gucharmap/gucharmap.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <cstdlib>

static const StarDictPluginSystemService *plugin_service;


static std::string get_codepoint(gunichar uc)
{
	std::string codepoint;
	gchar *temp = g_strdup_printf ("U+%4.4X %s", uc, gucharmap_get_unicode_name (uc));
	codepoint += "<kref>";
	codepoint += temp;
	codepoint += "</kref>";
	g_free(temp);
	return codepoint;
}

#define is_hex_digit(c) (((c) >= '0' && (c) <= '9') \
                         || ((c) >= 'A' && (c) <= 'F'))

/* - "XXXX-YYYY" used in annotation of U+003D 
 * - Annotation of U+03C0 uses ".XXXX" as a fractional number,
 *   so don't add "." to the list.
 * Add here if you know more. */
#define is_blank(c) (  ((c) == ' ')     \
                    || ((c) == '-') )

#define is_blank_or_hex_or(a,b) (  !((a) < len) \
                                || is_blank(str[a])     \
                                || (is_hex_digit(str[a]) && (b)) )

/* returns a pointer to the start of (?=[^ -])[0-9A-F]{4,5,6}[^0-9A-F],
 * or null if not found */
static const gchar *find_codepoint (const gchar *str)
{
	guint i, len;

	/* what we are searching for is ascii; in this case, we don't have to
	* worry about multibyte characters at all */
	len = strlen (str);
	for (i = 0;  i + 3 < len;  i++)
	{
		if ( ( !(i > 0) || is_blank(str[i-1]) )
		&& is_hex_digit (str[i+0]) && is_hex_digit (str[i+1])
		&& is_hex_digit (str[i+2]) && is_hex_digit (str[i+3])
		&& is_blank_or_hex_or(i+4, is_blank_or_hex_or(i+5,
		(i+6 < len) || !is_hex_digit (str[i+6]))) )
			return str + i;
	}
	return NULL;
}

static std::string get_string_link_codepoints(const gchar *str)
{
	std::string definition;
	const gchar *p1, *p2;
	p1 = str;
	char *mark;
	for (;;) {
		p2 = find_codepoint (p1);
		if (p2 != NULL) {
			mark = g_markup_escape_text(p1, p2 - p1);
			definition.append(mark);
			g_free(mark);
			gunichar uc;
			uc = strtoul (p2, (gchar **) &p1, 16);
			definition += get_codepoint(uc);
		} else {
			definition += p1;
			break;
		}
	}
	return definition;
}

static std::string get_chocolate_detail(const gchar *name, const gchar **values, gboolean expand_codepoints)
{
	std::string detail;
	detail += name;
	detail += "\n";
	for (gint i = 0;  values[i];  i++) {
		detail += " • ";
		if (expand_codepoints)
			detail += get_string_link_codepoints(values[i]);
		else
			detail += values[i];
		detail += "\n";
	}
	detail += "\n";
	return detail;
}

static std::string get_chocolate_detail_codepoints(const gchar *name, const gunichar *ucs)
{
	std::string detail;
	detail += name;
	detail += "\n";
	for (gint i = 0;  ucs[i] != (gunichar)(-1);  i++) {
		detail += " • ";
		detail += get_codepoint(ucs[i]);
		detail += "\n";
	}
	detail += "\n";
	return detail;
}

static std::string get_vanilla_detail(const gchar *name, const gchar *value)
{
	std::string detail;
	detail += name;
	detail += " ";
	detail += value;
	detail += "\n";
	return detail;
}

static int hexalpha_to_int(int c)
{
	char hexalpha[] = "aAbBcCdDeEfF";
	int i;
	int answer = 0;

	for(i = 0; answer == 0 && hexalpha[i] != '\0'; i++) {
		if(hexalpha[i] == c) {
			answer = 10 + (i / 2);
		}
	}
	return answer;
}

static unsigned int htoi(const char s[])
{
	unsigned int answer = 0;
	int i = 0;
	int hexit;

	while(s[i] != '\0') {
		if(s[i] >= '0' && s[i] <= '9') {
			answer = answer * 16;
			answer = answer + (s[i] - '0');
		} else {
			hexit = hexalpha_to_int(s[i]);
			if(hexit == 0) {
				break;
			} else {
				answer = answer * 16;
				answer = answer + hexit;
			}
		}
		++i;
	}
	return answer;
}

static void lookup(const char *text, char ***pppWord, char ****ppppWordData)
{
	if ((text[0] == '&' && text[1] == '#' && g_str_has_suffix(text, ";")) || g_str_has_prefix(text, "U+")) {
		gunichar uc;
		if (text[0] == '&') {
			if (text[2] == 'x' || text[2] == 'X') {
				uc = htoi(text+3);
			} else {
				uc = atoi(text+2);
			}
		} else { // U+
			uc = htoi(text+2);
		}
		gchar utf8[7];
		gint n = g_unichar_to_utf8(uc, utf8);
		utf8[n] = '\0';

		*pppWord = (gchar **)g_malloc(sizeof(gchar *)*2);
		(*pppWord)[0] = g_strdup(text);
		(*pppWord)[1] = NULL;
		*ppppWordData = (gchar ***)g_malloc(sizeof(gchar **)*(1));
		(*ppppWordData)[0] = (gchar **)g_malloc(sizeof(gchar *)*2);
		(*ppppWordData)[0][0] =  plugin_service->build_dictdata('m', utf8);
		(*ppppWordData)[0][1] = NULL;
		return;
	}
	bool found;
	gunichar uc;
	if (g_utf8_strlen(text, -1) != 1) {
		found = false; // Don't query it.
	} else {
		uc = g_utf8_get_char(text);
		if (!gucharmap_unichar_validate (uc) || !gucharmap_unichar_isdefined (uc))
			found = false;
		else
			found = true;
	}
	if (!found) {
		*pppWord = NULL;
		return;
	}
	std::string definition;
	definition += "\n";
	gchar buf[12];
	int n = gucharmap_unichar_to_printable_utf8 (uc, buf);
	if (n == 0) {
		definition += _("[not a printable character]");
	} else {
		gchar *str = g_markup_escape_text(buf, n);
		definition += "<big><big><big><big>";
		definition += str;
		definition += "</big></big></big></big>";
		g_free(str);
	}
	definition += "\n\n";
	gchar *temp;
	/* character name */
	temp = g_strdup_printf ("U+%4.4X %s", uc, gucharmap_get_unicode_name (uc));
	definition += "<big><b>";
	definition += temp;
	definition += "</b></big>\n";
	g_free (temp);
	definition += "\n<b>";
	definition += _("General Character Properties");
	definition += "</b>\n\n";
	/* character category */
	definition += get_vanilla_detail(_("Unicode category:"), gucharmap_get_unicode_category_name (uc));
	/* canonical decomposition */
	gunichar decomposition[G_UNICHAR_MAX_DECOMPOSITION_LENGTH];
	gsize result_len;
	result_len = g_unichar_fully_decompose(uc, FALSE, decomposition, G_UNICHAR_MAX_DECOMPOSITION_LENGTH);
	if (result_len != 1) {
		definition += _("Canonical decomposition:");
		definition += " ";
		definition += get_codepoint(decomposition[0]);
		for (gsize i = 1;  i < result_len;  i++) {
			definition += " + ";
			definition += get_codepoint(decomposition[i]);
		}
		definition += "\n";
	}
	/* representations */
	if (g_unichar_break_type(uc) != G_UNICODE_BREAK_SURROGATE) {
		definition += "\n<b>";
		definition += _("Various Useful Representations");
		definition += "</b>\n\n";
		guchar utf8[7];
		gunichar2 *utf16;
		GString *gstemp;
		n = g_unichar_to_utf8 (uc, (gchar *)utf8);
		utf16 = g_ucs4_to_utf16 (&uc, 1, NULL, NULL, NULL);
		/* UTF-8 */
		gstemp = g_string_new (NULL);
		gint i;
		for (i = 0;  i < n;  i++)
			g_string_append_printf (gstemp, "0x%2.2X ", utf8[i]);
		g_string_erase (gstemp, gstemp->len - 1, -1);
		definition += get_vanilla_detail(_("UTF-8:"), gstemp->str);
		g_string_free (gstemp, TRUE);
		/* UTF-16 */
		gstemp = g_string_new (NULL);
		g_string_append_printf (gstemp, "0x%4.4X", utf16[0]);
		if (utf16[0] != '\0' && utf16[1] != '\0')
			g_string_append_printf (gstemp, " 0x%4.4X", utf16[1]);
		definition += get_vanilla_detail(_("UTF-16:"), gstemp->str);
		g_string_free (gstemp, TRUE);
		/* an empty line */
		definition += "\n";
		/* C octal \012\234 */
		gstemp = g_string_new (NULL);
		for (i = 0;  i < n;  i++)
			g_string_append_printf (gstemp, "\\%3.3o", utf8[i]);
		definition += get_vanilla_detail(_("C octal escaped UTF-8:"), gstemp->str);
		g_string_free (gstemp, TRUE);
		/* XML entity */
		if ((0x0001 <= uc && uc <= 0xD7FF) ||
			(0xE000 <= uc && uc <= 0xFFFD) ||
			(0x10000 <= uc && uc <= 0x10FFFF))
		{
			temp = g_strdup_printf ("<kref>&amp;#%d;</kref>", uc);
			definition += get_vanilla_detail(_("XML decimal entity:"), temp);
			g_free (temp);
			temp = g_strdup_printf ("<kref>&amp;#x%X;</kref>", uc);
			definition += get_vanilla_detail(_("XML hexadecimal entity:"), temp);
			g_free (temp);
		}
		g_free(utf16);
	}
	/* annotations */
	std::string annotations;
	/* nameslist equals (alias names) */
	const gchar **csarr;
	csarr = gucharmap_get_nameslist_equals (uc);
	if (csarr != NULL) {
		annotations += get_chocolate_detail(_("Alias names:"), csarr, FALSE);
		g_free (csarr);
	}
	/* nameslist stars (notes) */
	csarr = gucharmap_get_nameslist_stars (uc);
	if (csarr != NULL) {
		annotations += get_chocolate_detail(_("Notes:"), csarr, TRUE);
		g_free (csarr);
	}
	/* nameslist exes (see also) */
	gunichar *ucs;
	ucs = gucharmap_get_nameslist_exes (uc);
	if (ucs != NULL) {
		annotations += get_chocolate_detail_codepoints(_("See also:"), ucs);
		g_free (ucs);
	}
	/* nameslist pounds (approximate equivalents) */
	csarr = gucharmap_get_nameslist_pounds (uc);
	if (csarr != NULL) {
		annotations += get_chocolate_detail(_("Approximate equivalents:"), csarr, TRUE);
		g_free (csarr);
	}
	/* nameslist colons (equivalents) */
	csarr = gucharmap_get_nameslist_colons (uc);
	if (csarr != NULL) {
		annotations += get_chocolate_detail(_("Equivalents:"), csarr, TRUE);
		g_free (csarr);
	}
	if (!annotations.empty()) {
		definition += "\n<b>";
		definition += _("Annotations and Cross References");
		definition += "</b>\n\n";
		definition += annotations;
	}
	std::string unihan;
	const gchar *csp = gucharmap_get_unicode_kDefinition (uc);
	if (csp)
		unihan += get_vanilla_detail(_("Definition in English:"), csp);
	csp = gucharmap_get_unicode_kMandarin (uc);
	if (csp)
		unihan += get_vanilla_detail(_("Mandarin Pronunciation:"), csp);
	csp = gucharmap_get_unicode_kCantonese (uc);
	if (csp)
		unihan += get_vanilla_detail(_("Cantonese Pronunciation:"), csp);
	csp = gucharmap_get_unicode_kJapaneseOn (uc);
	if (csp)
		unihan += get_vanilla_detail(_("Japanese On Pronunciation:"), csp);
	csp = gucharmap_get_unicode_kJapaneseKun (uc);
	if (csp)
		unihan += get_vanilla_detail(_("Japanese Kun Pronunciation:"), csp);
	csp = gucharmap_get_unicode_kTang (uc);
	if (csp)
		unihan += get_vanilla_detail(_("Tang Pronunciation:"), csp);
	csp = gucharmap_get_unicode_kKorean (uc);
	if (csp)
		unihan += get_vanilla_detail(_("Korean Pronunciation:"), csp);
	if (!unihan.empty()) {
		definition += "\n<b>";
		definition += _("CJK Ideograph Information");
		definition += "</b>\n\n";
		definition += unihan;
	}
	n = definition.length();
	int l = n-1;
	while (l >= 0 && definition[l] == '\n') {
		l--;
	}
	if (l < n-1) {
		definition.erase(l+1, n-1-l);
	}
	*pppWord = (gchar **)g_malloc(sizeof(gchar *)*2);
	(*pppWord)[0] = g_strdup(text);
	(*pppWord)[1] = NULL;
	*ppppWordData = (gchar ***)g_malloc(sizeof(gchar **)*(1));
	(*ppppWordData)[0] = (gchar **)g_malloc(sizeof(gchar *)*2);
	(*ppppWordData)[0][0] =  plugin_service->build_dictdata('x', definition.c_str());
	(*ppppWordData)[0][1] = NULL;
}

static void configure()
{
}

bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs)
{
	g_debug(_("Loading Gucharmap plug-in..."));
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print(_("Error: Gucharmap plugin version doesn't match!\n"));
		return true;
	}
	obj->type = StarDictPlugInType_VIRTUALDICT;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng001@gmail.com&gt;</author><website>http://stardict-4.sourceforge.net</website></plugin_info>", _("Gucharmap"), _("Gucharmap virtual dictionary."), _("Show information about Unicode characters."));
	obj->configure_func = configure;
	plugin_service = obj->plugin_service;

	return false;
}

void stardict_plugin_exit(void)
{
}

bool stardict_virtualdict_plugin_init(StarDictVirtualDictPlugInObject *obj)
{
	obj->lookup_func = lookup;
	obj->dict_name = _("Gucharmap");
	g_print(_("Gucharmap plug-in loaded.\n"));
	return false;
}
