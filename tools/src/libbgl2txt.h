#ifndef _LIBBGL2TXT_H_
#define _LIBBGL2TXT_H_

#include <string>
#include "libcommon.h"

extern void convert_bglfile(const std::string& infile, const std::string& outfile,
		const std::string& source_charset,
		const std::string& target_charset);

#endif
