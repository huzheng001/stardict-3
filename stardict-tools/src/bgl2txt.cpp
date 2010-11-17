#include <cstdio>
#include <cstring>
#include <glib.h>
#include "libbgl2txt.h"
#include "libcommon.h"

int main(int argc, char * argv[])
{
	if (argc<2) {
		printf("please type this:\n./bgl2txt [-s <source charset>] [-t <target charset>] a.bgl\n");
		return false;
	}
	std::string infile, source_charset, target_charset;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-s")== 0) {
			i++;
			source_charset = argv[i];
		} else if (strcmp(argv[i], "-t")==0) {
			i++;
			target_charset = argv[i];
		} else {
			infile = argv[i];
		}
	}
	std::string outfile = get_basename_without_extension(infile) + ".babylon";
	convert_bglfile(infile, outfile, source_charset, target_charset, g_print);
	return false;
}
