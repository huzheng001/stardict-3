#include "bgl_babylonreader.h"
#include "bgl_stardictbuilder.h"

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
	std::string outfile;
	const char *p = infile.c_str();
	const char *p1 = strrchr(p, '.');
	if (p1) {
		outfile.assign(p, p1-p);
	} else {
		outfile = infile;
	}
	DictBuilder *builder = new StarDictBuilder( outfile );
	DictReader *reader = new BabylonReader( infile, builder );
	if( !reader->convert(source_charset, target_charset) ) {
		printf( "Error converting %s\n", infile.c_str() );
		exit(1);
	}
	builder->finish();
	return false;
}
