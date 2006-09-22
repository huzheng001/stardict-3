#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstdlib>
#define private public

#include "articleview.h"

int main(int argc, char *argv[])
{
	const char *ar = "<k>nick</k> <k>name</k>\n"
		"<tr>neim</tr>\n"
		"<abr>noun.</abr> <co>In the rest of article we used latinitsu</co>\n"
		"<b>nick</b>, <b>Imya</b>, <i>Italic</i>, <c code=\"green\">color</c>\n"
		"<ex>My name is Vova.</ex> <c>the</c> rest\n"
		"of article.";
	const char *after =
		" <b>[neim]</b>\n"
		"<span foreground=\"green\" style=\"italic\">noun.</span> "
		"In the rest of article we used latinitsu\n"
		"<b>nick</b>, <b>Imya</b>, <i>Italic</i>, <span foreground=\"green\">color</span>\n"
		"<span foreground=\"violet\">My name is Vova.</span> "
		"<span foreground=\"blue\">the</span> rest\n"
		"of article.";
	std::string res = ArticleView::xdxf2pango(ar);
	if (res != after) {
		g_warning("%s: we got not what we expected: %s", argv[0], res.c_str());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
