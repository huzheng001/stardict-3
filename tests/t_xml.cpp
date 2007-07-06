#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <cstdlib>
#include <string>


static void xml_decode(const char *str, std::string& decoded)
{
	static const char raw_entrs[] = { 
		'<',   '>',   '&',    '\'',    '\"',    0 
	};
	static const char* xml_entrs[] = { 
		"lt;", "gt;", "amp;", "apos;", "quot;", 0 
	};
	static const int xml_ent_len[] = { 
		3,     3,     4,      5,       5 
	};
	int ient;
        const char *amp = strchr(str, '&');

        if (amp == NULL) {
		decoded = str;
                return;
        }
        decoded.assign(str, amp - str);
        
        while (*amp)
                if (*amp == '&') {
                        for (ient = 0; xml_entrs[ient] != 0; ++ient)
                                if (strncmp(amp + 1, xml_entrs[ient],
					    xml_ent_len[ient]) == 0) {
                                        decoded += raw_entrs[ient];
                                        amp += xml_ent_len[ient]+1;
                                        break;
                                }
                        if (xml_entrs[ient] == 0)    // unrecognized sequence
                                decoded += *amp++;

                } else {
                        decoded += *amp++;
                }        
}

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
