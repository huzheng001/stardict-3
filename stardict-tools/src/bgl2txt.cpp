#include "libbgl2txt.h"

int main(int argc, char * argv[])
{
	if (argc<2) {
		printf("please type this:\n./bgl2txt a.bgl\n");
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
	convert_bglfile(infile, source_charset, target_charset);
	return false;
}
