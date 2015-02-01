/*
 * This file is part of makedict - convertor from any
 * dictionary format to any http://xdxf.sourceforge.net
 *
 * Copyright (C) Evgeniy Dushistov, 2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_MMAP
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <unistd.h>
#elif defined(WIN32)
# include <windows.h>
#else
# include <glib.h>
#endif

#include <cerrno>
#include <cstring>
#include <glib.h>
#include <glib/gi18n.h>

#include "utils.h"
#include "log.h"

#include "mapfile.h"

MapFile::MapFile() : cur(NULL), data(NULL), end_of_file(NULL), size(0)
{
#ifdef HAVE_MMAP
	mmap_fd=-1;
#endif
}

void MapFile::close(void)
{
	if (!data)
		return;
#ifdef HAVE_MMAP
	munmap(data, size);
	::close(mmap_fd);
	mmap_fd=-1;
#elif defined(WIN32)
	UnmapViewOfFile(data);
	CloseHandle(hFileMap);
	CloseHandle(hFile);
#endif
	data=NULL;
}

#if defined(WIN32)
wchar_t *convertCharArrayToLPCWSTR(const char* charArray)
{
	wchar_t* wString = new wchar_t[4096];
	MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, 4096);
	return wString;
}
#endif

bool MapFile::open(const char *file_name, bool logerr, long file_size)
{
	filename_ = file_name;

	close();
	size = file_size;
#ifdef HAVE_MMAP
	if ((mmap_fd = ::open(file_name, O_RDONLY)) < 0) {
		if (logerr)
			g_warning(_("Can not open %s: %s\n"), file_name, strerror(errno));
		else
			g_info(_("Can not open %s: %s\n"), file_name, strerror(errno));
		return false;
	}
	
	if (-1 == size) {
		struct stat stat_info;
		if (fstat(mmap_fd, &stat_info)==-1)
			return false;
		size = stat_info.st_size;
	}
	data = (char *)mmap(NULL, size, PROT_READ, MAP_SHARED, mmap_fd, 0);
	if ((void *)data == (void *)(-1))
		return false;

#elif defined(WIN32)
	//TODO: rewirte to handle CreateFileA and CreateFileW cases
	hFile = CreateFile(convertCharArrayToLPCWSTR(file_name), GENERIC_READ, 0, NULL, OPEN_ALWAYS,
			   FILE_ATTRIBUTE_NORMAL, 0);
	if (-1==size)
		size=GetFileSize(hFile, NULL);
	hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0,
				     size, NULL);
	data = (char *)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, size);
#  else
	gsize fsize;
	if (!g_file_get_contents(file_name, get_addr(databuf_), &fsize, NULL))
		return false;
	size = fsize;
	data = get_impl(databuf_);
#endif
	cur = data;
	end_of_file = data + size;
	return true;
}

MapFile::~MapFile()
{
	close();
}

char *MapFile::find_str(char *beg, const char *str, char *end)
{
	const char *p;
	if (!end)
		end=MapFile::end();
	while (beg!=end) {
		p=str;
		while (beg!=end && *p && *p==*beg) ++p, ++beg;
		if (*p=='\0')
			return beg+(str-p);
		++beg;
	}
	return end;
}

