#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "lib.h"

query_t analyse_query(const char *s, std::string& res)
{
	if (!s || !*s) {
		res="";
		return qtSIMPLE;
	}
	if (*s=='/') {
		res=s+1;
		return qtFUZZY;
	}
	if (*s==':') {
		res=s+1;
		return qtREGEX;
	}

	if (*s=='|') {
		res=s+1;
		return qtDATA;
	}

	bool pattern=false;
	const char *p=s;
	res="";
	for (; *p; res+=*p, ++p) {
		if (*p=='\\') {
			++p;
			if (!*p)
				break;
			continue;
		}
		if (*p=='*' || *p=='?')
			pattern=true;
	}
	if (pattern)
		return qtPATTERN;

	return qtSIMPLE;
}

void stardict_input_escape(const char *text, std::string &res)
{
	res.clear();
	const char *p = text;
	if (*p == '/' || *p == '|' || *p == ':') {
		res = "\\";
		res += *p;
		p++;
	}
	while (*p) {
		if (*p == '\\' || *p == '*' || *p == '?') {
			res += '\\';
			res += *p;
		} else {
			res += *p;
		}
		p++;
	}
}
