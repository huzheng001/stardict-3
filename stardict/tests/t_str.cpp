#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include "lib/utils.h"


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

int main(int argc, char *argv[])
{
	test_extract_word();
	test_extract_capitalized_word();
	test_copy_normalize_trim_spaces();
}
