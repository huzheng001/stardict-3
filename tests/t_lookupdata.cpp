#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstdlib>
#include <vector>
#include <iostream>

#include "file.hpp"
#include "lib.h"

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);
	
	Libs libs(NULL, false, false, 0);
	List dict_list;
	libs.load(dict_list);
	std::vector<InstantDictIndex> dictmask;
	std::vector<gchar *> reslist[dictmask.size()];
	if (libs.LookupData("letter", reslist, NULL, NULL, NULL, dictmask)) 
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}
