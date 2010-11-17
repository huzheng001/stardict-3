#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include "libbgl2txt.h"
#include "bgl_babylonreader.h"
#include "bgl_stardictbuilder.h"

void convert_bglfile(const std::string& infile, const std::string& outfile,
		const std::string& source_charset,
		const std::string& target_charset, print_info_t print_info)
{
	StarDictBuilder builder( outfile, print_info );
	BabylonReader reader( infile, outfile, &builder, print_info );
	if( !reader.convert(source_charset, target_charset) ) {
		print_info( "Error converting %s\n", infile.c_str() );
		return;
	}
	builder.finish();
}
