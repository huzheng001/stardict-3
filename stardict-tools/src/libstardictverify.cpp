#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <cstdlib>
#include <glib/gstdio.h>
#include <glib.h>
#include <string>
#include <vector>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include "libstardictverify.h"
#include "libcommon.h"
#include "resourcewrap.hpp"
#include "lib_res_store.h"
#include "lib_norm_dict.h"

/* Terminology

Index file is a sequence of index items.
An index item consists of:
word - the key of the item;
size and offset of data block containing definition in the dictionary file.
A data block consists of a number of fields.
A field has a type specified by type identifier (one character, an ASCII letter).
*/

int stardict_verify(const char *ifofilename, print_info_t print_info)
{
	bool have_errors = false;

	glib::CharStr cdirname(g_path_get_dirname(ifofilename));
	resource_storage res_storage;
	if(lrError == res_storage.load(get_impl(cdirname), print_info))
		have_errors = true;

	norm_dict dict;
	if(EXIT_FAILURE == dict.load(ifofilename, print_info, static_cast<i_resource_storage*>(&res_storage)))
		have_errors = true;

	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
