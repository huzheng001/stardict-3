// Come from: http://code.google.com/p/dazhiqian/source/browse/trunk/C/dictd-dabase/sd2foldoc/

#include <glib.h>

#include "libsd2foldoc.h"

using namespace std;

void print_info(const char *info)
{
        g_print("%s", info);
}

int main(int argc,char * argv [])
{
	if (argc!=2) {
		g_print("please type this:\n./sd2foldoc somedict.ifo\n");
	} else {
		sd2foldoc(argv[1], print_info);
	}
	return TRUE;
}
