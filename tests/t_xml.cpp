#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstdlib>

#include "utils.h"

static bool decode(const char *probe, const char *exp)
{
	std::string res;
	xml_decode(probe, res);
	if (res != exp) {
		g_warning("want: %s, got %s", exp, res.c_str());
		return false;
	}
	return true;
}

int main()
{
	const char *probe[] = {
		"aa &lt; &gt;&amp; &apos; &quot; bb",
		"",
		"aaa bbb",
		"aa &lt; &gt;&amp; &apos; &quot; bb, &b; &quot;",
		NULL
	};
	const char *expect[] = {
		"aa < >& ' \" bb",
		"",
		"aaa bbb",
		"aa < >& ' \" bb, &b; \"",
	};

	for (int i = 0; probe[i]; ++i) 
		if (!decode(probe[i], expect[i]))
			return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
