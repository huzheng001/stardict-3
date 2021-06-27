#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <string>

#include "libbgl2txt.h"
#include "bgl_babylonreader.h"
#include "bgl_stardictbuilder.h"

void convert_bglfile(const std::string& infile, const std::string& outfile,
		const std::string& source_charset,
		const std::string& target_charset)
{
	StarDictBuilder builder( outfile );
	BabylonReader reader( infile, outfile, &builder );
	if( !reader.convert(source_charset, target_charset) ) {
		g_critical("Error converting %s\n", infile.c_str());
		return;
	}
	builder.finish();
}
