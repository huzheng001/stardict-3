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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include "lib/utils.h"
#include "libcommon.h"


void test_extract_word(void)
{
	const size_t buffer_size = 256;
	char buffer[buffer_size];
	struct TData {
		const char* source;
		int pos;
		const char* result;
	};
	TData data[] = {
		{
			"",
			0,
			""
		},
		{
			"        ",
			2,
			""
		},
		{
			"aaaaaaa",
			0,
			"aaaaaaa"
		},
		{
			"aaaaaaa",
			1,
			"aaaaaaa"
		},
		{
			"  aaaaaaa",
			0,
			"aaaaaaa"
		},
		{
			"  aaaaaaa  ",
			4,
			"aaaaaaa"
		},
		{
			" aaa   bbbb ",
			5,
			"aaa"
		},
		{
			" 1111   333333  33333  7 9 00000  88383883 ",
			24,
			"7"
		},

		{
			NULL,
			0,
			NULL
		}
	};
	for(TData *d=data; d->source; ++d) {
		extract_word(buffer, d->source, d->pos, g_unichar_isspace);
		if(strcmp(buffer, d->result)) {
			printf("Test extract_word failed. source = %s, pos = %d\n", d->source, d->pos);
			exit(1);
		}
	}
}

void test_extract_capitalized_word(void)
{
	const size_t buffer_size = 256;
	char buffer[buffer_size];
	struct TData {
		const char* source;
		int pos;
		const char* result;
	};
	TData data[] = {
		{
			"",
			0,
			""
		},
		{
			"cAb     ",
			0,
			"Ab"
		},
		{
			"cAb     ",
			1,
			"Ab"
		},
		{
			"cAb     ",
			2,
			"Ab"
		},
		{
			"cAb     ",
			4,
			"Ab"
		},

		{
			NULL,
			0,
			NULL
		}
	};

	for(TData *d=data; d->source; ++d) {
		extract_capitalized_word(buffer, d->source, d->pos, g_unichar_isupper, g_unichar_islower);
		if(strcmp(buffer, d->result)) {
			printf("Test extract_capitalized_word failed. source = %s, pos = %d\n", d->source, d->pos);
			exit(1);
		}
	}
}

void test_copy_normalize_trim_spaces(void)
{
	const size_t buffer_size = 256;
	char buffer[buffer_size];
	struct TData {
		const char* source;
		const char* result;
	};
	TData data[] = {
		{
			"   a   b ",
			"a b"
		},
		{
			"a  b  d",
			"a b d"
		},
		
		{
			NULL,
			NULL
		}
	};
	for(TData *d=data; d->source; ++d) {
		copy_normalize_trim_spaces(buffer, d->source);
		if(strcmp(buffer, d->result)) {
			printf("Test copy_normalize_trim_spaces failed. source = %s\n", d->source);
			exit(1);
		}
	}
}


void test_norm_path_win(void)
{
	struct TData {
		const char* source;
		const char* resolved_path;
		int result;
	};
	TData data[] = {
		// all supported roots
		{
			"c:\\",
			"c:\\",
			EXIT_SUCCESS
		},
		{
			"c:",
			"c:",
			EXIT_SUCCESS
		},
		{
			"\\\\abcd",
			"\\\\abcd",
			EXIT_SUCCESS
		},
		{
			"\\\\abcd\\",
			"\\\\abcd\\",
			EXIT_SUCCESS
		},
		{
			"\\\\",
			"",
			EXIT_FAILURE
		},
		{
			"\\\\\\",
			"",
			EXIT_FAILURE
		},
		{
			"\\abcd",
			"\\abcd",
			EXIT_SUCCESS
		},
		{
			"\\abcd\\",
			"\\abcd\\",
			EXIT_SUCCESS
		},
		{
			"abcd",
			"abcd",
			EXIT_SUCCESS
		},
		{
			"a",
			"a",
			EXIT_SUCCESS
		},
		{
			".",
			".",
			EXIT_SUCCESS
		},
		{
			".\\",
			".\\",
			EXIT_SUCCESS
		},
		{
			".\\ab",
			"ab",
			EXIT_SUCCESS
		},
		{
			"",
			".",
			EXIT_SUCCESS
		},
		// refering parent directory of the root
		{
			"c:\\..",
			"",
			EXIT_FAILURE
		},
		{
			"\\\\..",
			"\\\\..",
			EXIT_SUCCESS
		},
		{
			"\\\\abcd\\..",
			"",
			EXIT_FAILURE
		},
		{
			"\\..",
			"",
			EXIT_FAILURE
		},
		{
			"..",
			"..",
			EXIT_SUCCESS
		},
		{
			"..\\",
			"..\\",
			EXIT_SUCCESS
		},
		// empty path component
		{
			"c:\\ab\\\\cd",
			"",
			EXIT_FAILURE
		},
		{
			"\\\\abcd\\\\",
			"",
			EXIT_FAILURE
		},
		{
			"\\abcd\\\\de",
			"",
			EXIT_FAILURE
		},
		// resolving parent references
		{
			"..",
			"..",
			EXIT_SUCCESS
		},
		{
			"..\\..",
			"..\\..",
			EXIT_SUCCESS
		},
		{
			"abcd\\..",
			".",
			EXIT_SUCCESS
		},
		{
			"abcd\\..\\..",
			"..",
			EXIT_SUCCESS
		},
		{
			"abcd\\..\\..\\",
			"..\\",
			EXIT_SUCCESS
		},
		{
			"\\a\\b\\..",
			"\\a\\",
			EXIT_SUCCESS
		},
		{
			"\\a\\b\\c\\..\\..\\d",
			"\\a\\d",
			EXIT_SUCCESS
		},
		{
			".\\..",
			"..",
			EXIT_SUCCESS
		},
		// strip "." component
		{
			"c:\\dir\\.",
			"c:\\dir\\",
			EXIT_SUCCESS
		},
		{
			"c:\\dir\\.\\",
			"c:\\dir\\",
			EXIT_SUCCESS
		},
		{
			"\\dir\\.",
			"\\dir\\",
			EXIT_SUCCESS
		},
		{
			"\\dir\\.\\",
			"\\dir\\",
			EXIT_SUCCESS
		},
		{
			"\\.",
			"\\",
			EXIT_SUCCESS
		},
		{
			"\\.\\",
			"\\",
			EXIT_SUCCESS
		},
		{
			"\\dir\\.\\..",
			"\\",
			EXIT_SUCCESS
		},
		{
			"\\dir\\.\\.\\make",
			"\\dir\\make",
			EXIT_SUCCESS
		},

		/*
		{
			"",
			"",
			EXIT_SUCCESS
		},
		{
			"",
			"",
			EXIT_FAILURE
		},
		*/
		
		{
			NULL,
			NULL,
			0
		}
	};
	std::string resolved_path;
	for(TData *d=data; d->source; ++d) {
		int result = norm_path_win(d->source, resolved_path);
		if(strcmp(resolved_path.c_str(), d->resolved_path) != 0 || result != d->result) {
			printf("Test test_resolve_path failed. source = %s\n", d->source);
			exit(1);
		}
	}
}

void test_build_relative_path(void)
{
	struct TData {
		const char* base_dir;
		const char* path;
		const char* rel_path;
		int result;
	};
	TData data[] = {
		{
			"c:\\",
			"c:\\dir1",
			"dir1",
			EXIT_SUCCESS
		},
		{
			"c:\\",
			"c:\\dir1\\",
			"dir1\\",
			EXIT_SUCCESS
		},
		{
			"c:\\",
			"c:\\a\\b\\",
			"a\\b\\",
			EXIT_SUCCESS
		},
		{
			"c:\\",
			"c:\\a\\b",
			"a\\b",
			EXIT_SUCCESS
		},
		{
			"c:\\dir",
			"c:\\dir",
			"",
			EXIT_SUCCESS
		},
		{
			"c:\\dir\\",
			"c:\\dir",
			"",
			EXIT_SUCCESS
		},
		{
			"c:\\dir\\",
			"c:\\dir\\",
			"",
			EXIT_SUCCESS
		},
		{
			"\\",
			"\\a",
			"a",
			EXIT_SUCCESS
		},
		{
			"\\",
			"\\a\\",
			"a\\",
			EXIT_SUCCESS
		},
		{
			"\\a",
			"\\a\\",
			"",
			EXIT_SUCCESS
		},
		{
			"\\\\a",
			"\\\\a",
			"",
			EXIT_SUCCESS
		},
		{
			"\\\\a\\",
			"\\\\a",
			"",
			EXIT_SUCCESS
		},
		{
			"\\\\a",
			"\\\\a\\dir",
			"dir",
			EXIT_SUCCESS
		},
		{
			"\\\\a\\",
			"\\\\a\\dir",
			"dir",
			EXIT_SUCCESS
		},
		{
			"\\\\a\\",
			"\\\\a\\dir\\",
			"dir\\",
			EXIT_SUCCESS
		},
		{
			"\\a\\b\\c",
			"\\a\\b",
			"..",
			EXIT_SUCCESS
		},
		{
			"\\a\\b\\c\\",
			"\\a\\b",
			"..",
			EXIT_SUCCESS
		},
		{
			"\\a\\b\\c\\",
			"\\a\\b\\",
			"..\\",
			EXIT_SUCCESS
		},
		{
			"\\a\\b\\c",
			"\\a\\d",
			"..\\..\\d",
			EXIT_SUCCESS
		},
		{
			"\\a\\b\\c",
			"\\a\\d\\",
			"..\\..\\d\\",
			EXIT_SUCCESS
		},

		/*
		{
			"",
			"",
			"",
			EXIT_SUCCESS
		},
		{
			"",
			"",
			"",
			EXIT_FAILURE
		},
		*/

		{
			NULL,
			NULL,
			NULL,
			0
		}
	};
	std::string rel_path;
	for(TData *d=data; d->base_dir; ++d) {
		int result = build_relative_path(d->base_dir, d->path, rel_path);
		if(strcmp(rel_path.c_str(), d->rel_path) != 0 || result != d->result) {
			printf("Test test_build_relative_path failed. base_dir = '%s', path = '%s', rel_path = '%s'\n", 
				d->base_dir, d->path, rel_path.c_str());
			exit(1);
		}
	}
}

void test_is_ascii_alpha(void)
{
	struct TData {
		wchar_t ch;
		bool result;
	};
	TData data[] = {
		{
			L'a',
			true
		},
		{
			L'Z',
			true
		},
		/*
		{
			L'',
			true
		},
		{
			L'',
			false
		},
		*/
		{
			L'\0',
			false
		}
	};
	for(TData *d=data; d->ch; ++d) {
		bool result = is_ascii_alpha(d->ch);
		if(result != d->result) {
			printf("Test test_is_ascii_alpha failed.\n");
			exit(1);
		}
	}
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	test_extract_word();
	test_extract_capitalized_word();
	test_copy_normalize_trim_spaces();
	test_norm_path_win();
	test_build_relative_path();
	test_is_ascii_alpha();
	return 0;
}
