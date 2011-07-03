/*
 * Copyright 2011 kubtek <kubtek@mail.com>
 *
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __DICT_ZIP_LIB_H__
#define __DICT_ZIP_LIB_H__

#include <ctime>
#include <string>
#include <zlib.h>

#include "mapfile.h"


#define DICT_CACHE_SIZE 5

struct dictCache {
	int           chunk;
	char          *inBuffer;
	int           stamp;
	int           count;
};

struct dictData {
	dictData() {}
	bool open(const std::string& filename, int computeCRC);
	void close();
	void read(char *buffer, unsigned long start, unsigned long size);
	~dictData() { close(); }
private:
	const char    *start;	/* start of mmap'd area */
	const char    *end;		/* end of mmap'd area */
	unsigned long size;		/* size of mmap */
	
	int           type;
	z_stream      zStream;
	int           initialized;
  
	int           headerLength;
	int           method;
	int           flags;
	time_t        mtime;
	int           extraFlags;
	int           os;
	int           version;
	int           chunkLength;
	int           chunkCount;
	int           *chunks;
	unsigned long *offsets;	/* Sum-scan of chunks. */
	std::string    origFilename;
	std::string    comment;
	unsigned long crc;
	unsigned long length;
	unsigned long compressedLength;
	dictCache     cache[DICT_CACHE_SIZE];
	MapFile mapfile;

	int read_header(const std::string &filename, int computeCRC);
};

#endif//!__DICT_ZIP_LIB_H__
