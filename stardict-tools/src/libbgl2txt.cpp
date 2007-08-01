#include "libbgl2txt.h"
#include "bgl_babylonreader.h"
#include "bgl_stardictbuilder.h"

void convert_bglfile(std::string infile, std::string source_charset, std::string target_charset)
{
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
		return;
	}
	builder->finish();
}
