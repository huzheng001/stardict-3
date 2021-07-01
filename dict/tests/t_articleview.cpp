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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <cstdlib>
#include <cstring>
#include <list>
#include <string>
#include "lib/parsedata_plugin.h"

static const char *xml_topos(const char *str, size_t pos)
{
	const char *q;
	static const char* xml_entrs[] = { "lt;", "gt;", "amp;", "apos;", "quot;", 0 };
	static const int xml_ent_len[] = { 3,     3,     4,      5,       5 };
	size_t cur_pos;
	int i;

	if (pos == 0)
		return str;

	for (cur_pos = 0, q = str; *q; ++cur_pos) {
		if (cur_pos == pos)
			return q;
		if (*q == '&') {
			for (i = 0; xml_entrs[i]; ++i)
				if (strncmp(xml_entrs[i], q + 1,
					    xml_ent_len[i]) == 0) {
					q += xml_ent_len[i] + 1;
					break;
				}
			if (xml_entrs[i] == NULL)
				++q;
		} else if (*q == '<') {
			const char *p = strchr(q, '>');
			if (p)
				q = p + 1;
			else
				++q;
			--cur_pos;
		} else
			++q;
	}

	return NULL;
}

static bool check_xdxf2pango(const char *in, const char *out, 
			     const std::list<std::string>& links_list_std)
{
	LinksPosList links_list;
	//std::string res = articleview->xdxf2pango(in, "", links_list);
	std::string res;
	if (res != out) {
		g_warning("we got not what we expected:\n%s\n\n%s\n", res.c_str(), out);
		return false;
	}
	
	LinksPosList::const_iterator it;
	std::list<std::string>::const_iterator jt;

	for ( it = links_list.begin(), jt = links_list_std.begin();
	      it != links_list.end() && jt != links_list_std.end(); ++it, ++jt) {
		const char *link = xml_topos(res.c_str(), it->pos_);
		if (!link && std::string(link, it->len_) != *jt) {
			g_warning("we expect %s, got %s",
				  std::string(res, it->pos_, it->len_).c_str(),
				  jt->c_str());
			return false;
		}
	}

	if (!(it == links_list.end() && jt == links_list_std.end())) {
		g_warning("links detection error");
		return false;
	}
	return true;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	const char *ar1 = "<k>nick</k> <k>name</k>\n"
		"<tr>neim</tr>\n"
		"<abr>noun.</abr> <co>In the rest of article we used latinitsu</co>\n"
		"<b>nick</b>, <b><kref>Imya</kref></b>, <i>Italic</i>, <c c=\"green\">color</c>\n"
		"<ex>My name is <kref>Vova</kref>.</ex> <c>the</c> rest\n"
		"of article.";
	const char *after1 =
		" <span foreground=\"blue\">name</span>\n<b>[neim]</b>\n"
		"<span foreground=\"green\" style=\"italic\">noun.</span> "
		"In the rest of article we used latinitsu\n"
		"<b>nick</b>, <b><span foreground=\"blue\" underline=\"single\">Imya</span></b>, <i>Italic</i>, <span foreground=\"green\">color</span>\n"
		"<span foreground=\"violet\">My name is <span foreground=\"blue\" underline=\"single\">Vova</span>.</span> "
		"<span foreground=\"blue\">the</span> rest\n"
		"of article.";
	std::list<std::string> links_list_std;
	links_list_std.push_back("Imya");
	links_list_std.push_back("Vova");

	if (!check_xdxf2pango(ar1, after1, links_list_std))
		return EXIT_FAILURE;
	const char *ar2 = "<k>hello</k>\n"
		"<tr>ˈheˈləu</tr>\n"
		"<c><co>= <kref>hallo</kref></co></c>";
	const char *after2 = "<b>[ˈheˈləu]</b>\n"
		"<span foreground=\"blue\">= <span foreground=\"blue\" underline=\"single\">hallo</span></span>";
	links_list_std.clear();
	links_list_std.push_back("hallo");
	if (!check_xdxf2pango(ar2, after2, links_list_std))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
