#include "stardict_wiki2xml.h"
#include "WIKI2XML.h"

std::string wiki2xml(std::string &str)
{
	WIKI2XML w2x(str);
	w2x.parse () ;
	return w2x.get_xml ();
}
