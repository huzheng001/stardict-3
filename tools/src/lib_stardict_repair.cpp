/*
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
#include <stdlib.h>
#include <algorithm>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <errno.h>

#include "lib_stardict_repair.h"
#include "lib_dict_verify.h"
#include "lib_res_store.h"
#include "lib_binary_dict_parser.h"
#include "lib_common_dict.h"
#include "lib_binary_parser_unify.h"
#include "lib_binary_dict_generator.h"
#include "lib_dict_repair.h"

namespace gio {
	typedef ResourceWrapper<GFile, gpointer, void, g_object_unref> File;
}

static int copy_file(const std::string& srcdir, const std::string& dstdir, const std::string& filename)
{
	glib::Error error;
	std::string srcpath = build_path(srcdir, filename);
	std::string dstpath = build_path(dstdir, filename);
	gio::File srcfile(g_file_new_for_path(srcpath.c_str()));
	gio::File dstfile(g_file_new_for_path(dstpath.c_str()));
	if(!g_file_copy(get_impl(srcfile), get_impl(dstfile),
		GFileCopyFlags(G_FILE_COPY_OVERWRITE | G_FILE_COPY_TARGET_DEFAULT_PERMS),
		NULL, NULL, NULL, get_addr(error))) {
		g_critical(copy_file_err, srcpath.c_str(), dstpath.c_str(), error->message);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static int copy_dir(const std::string& srcdir, const std::string& dstdir)
{
	glib::Error error;
	if(g_mkdir_with_parents(dstdir.c_str(), S_IRWXU)) {
		std::string error(g_strerror(errno));
		g_critical(create_dir_err, dstdir.c_str(), error.c_str());
		return EXIT_FAILURE;
	}
	glib::Dir dir(g_dir_open(srcdir.c_str(), 0, get_addr(error)));
	if(!dir) {
		g_critical(open_dir_err, srcdir.c_str(), error->message);
		return EXIT_FAILURE;
	}
	const gchar *filename;
	while ((filename = g_dir_read_name(get_impl(dir)))!=NULL) {
		std::string srcitem(build_path(srcdir, filename));
		if (g_file_test(srcitem.c_str(), G_FILE_TEST_IS_DIR)) {
			std::string dstitem(build_path(srcdir, filename));
			if(copy_dir(srcitem, dstitem))
				return EXIT_FAILURE;
		} else {
			if(copy_file(srcdir, dstdir, filename))
				return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

static int copy_resource_storage(const resource_storage& res_storage, const std::string& srcdirpath,
		const std::string& dstdirpath)
{
	g_message("Copying resource storage...");
	if(res_storage.get_storage_type() == StorageType_DATABASE) {
		if(copy_file(srcdirpath, dstdirpath, "res.rifo"))
			return EXIT_FAILURE;
		if(res_storage.res_ridx_compressed()) {
			if(copy_file(srcdirpath, dstdirpath, "res.ridx.gz"))
				return EXIT_FAILURE;
		} else {
			if(copy_file(srcdirpath, dstdirpath, "res.ridx"))
				return EXIT_FAILURE;
		}
		if(res_storage.res_rdic_compressed()) {
			if(copy_file(srcdirpath, dstdirpath, "res.rdic.dz"))
				return EXIT_FAILURE;
		} else {
			if(copy_file(srcdirpath, dstdirpath, "res.rdic"))
				return EXIT_FAILURE;
		}
	} else if(res_storage.get_storage_type() == StorageType_FILE) {
		std::string srcpath = build_path(srcdirpath, "res");
		std::string dstpath = build_path(dstdirpath, "res");
		if(copy_dir(srcpath, dstpath))
			return EXIT_FAILURE;
	}
	g_message("Resource storage copied.");
	return EXIT_SUCCESS;
}

static int check_parameters(const std::string& ifofilepath, const std::string& outdirpath)
{
	if(!g_file_test(ifofilepath.c_str(), G_FILE_TEST_EXISTS)) {
		g_critical(file_not_found_err, ifofilepath.c_str());
		return EXIT_FAILURE;
	}
	if(!is_path_end_with(ifofilepath, ".ifo")) {
		g_critical(unsupported_file_type_err, ifofilepath.c_str());
		return EXIT_FAILURE;
	}
	if(!g_file_test(outdirpath.c_str(), G_FILE_TEST_EXISTS)) {
		g_critical(dir_not_found_err, outdirpath.c_str());
		return EXIT_FAILURE;
	}

	{	// check that outdirpath != dictionary directory
		glib::CharStr cdirname(g_path_get_dirname(ifofilepath.c_str()));
		stardict_stat_t stats1, stats2;
		if (g_stat (get_impl(cdirname), &stats1) == -1) {
			g_critical(dir_not_found_err, get_impl(cdirname));
			return EXIT_FAILURE;
		}
		if (g_stat (outdirpath.c_str(), &stats2) == -1) {
			g_critical(dir_not_found_err, outdirpath.c_str());
			return EXIT_FAILURE;
		}
		if(stats1.st_dev == stats2.st_dev && stats1.st_ino == stats2.st_ino) {
			g_critical("Output directory '%s' is identical to the dictionary directory '%s'. ",
				outdirpath.c_str(), get_impl(cdirname));
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

int stardict_repair(const std::string& ifofilepath, const std::string& outdirpath,
		RepairOptions options)
{
	if(check_parameters(ifofilepath, outdirpath))
		return EXIT_FAILURE;
	g_message("Loading dictionary: '%s'...", ifofilepath.c_str());

	bool have_resource_storage = false;
	glib::CharStr cdirname(g_path_get_dirname(ifofilepath.c_str()));
	resource_storage res_storage;
	TLoadResult lres = res_storage.load(get_impl(cdirname));
	if(lres == lrOK && res_storage.get_verif_result() <= VERIF_RESULT_WARNING)
		have_resource_storage = true;

	binary_dict_parser_t parser;
	parser.set_fix_errors(true);
	VerifResult vres = parser.load(ifofilepath,
		have_resource_storage ? static_cast<i_resource_storage*>(&res_storage) : NULL);
	if(VERIF_RESULT_FATAL <= vres) {
		g_critical("The dictionary '%s' cannot be repaired.", ifofilepath.c_str());
		return EXIT_FAILURE;
	}
	g_message("Dictionary loaded.");
	g_message("Checking and repairing the dictionary...");
	common_dict_t norm_dict;
	norm_dict.set_lot_of_memory(options.lot_of_memory);
	if(convert_to_parsed_dict(norm_dict, parser))
		return EXIT_FAILURE;
	if(repair_dict(norm_dict))
		return EXIT_FAILURE;
	g_message("Dictionary repaired.");

	glib::CharStr cifofilename(g_path_get_basename(ifofilepath.c_str()));
	std::string ifofilepath_out(build_path(outdirpath, get_impl(cifofilename)));
	binary_dict_gen_t generator;
	generator.set_use_same_type_sequence(true);
	generator.set_compress_dict(options.compress_dict);
	g_message("Saving dictionary in '%s'...", ifofilepath_out.c_str());
	if(generator.generate(ifofilepath_out, &norm_dict)) {
		g_critical("Save failed.");
		return EXIT_FAILURE;
	}
	g_message("Dictionary saved.");
	if(have_resource_storage && options.copy_res_store)
		copy_resource_storage(res_storage, get_impl(cdirname), outdirpath);

	return EXIT_SUCCESS;
}
