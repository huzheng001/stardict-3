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

#include <string.h>
#include <string>
#include <iostream>
#include <stack>
#include <vector>
#include <sstream>

#include <glib.h>
#include <glib/gstdio.h>
#include "libcommon.h"

class EMain {};

struct TIndexEntity {
	// database file name in utf8
	std::string db_file_name;
	// file system file name in file name encoding
	std::string fs_file_name;
	guint32 offset;
	guint32 size;
};

/* convert string from utf8 to file name encoding */
static bool Utf8ToFileName(const std::string& str, std::string& out)
{
	size_t len = str.length();
	gsize bytes_read, bytes_written;
	glib::Error err;
	glib::CharStr gstr(g_filename_from_utf8(str.c_str(), len, &bytes_read, 
		&bytes_written, get_addr(err)));
	if(!gstr || bytes_read != len) {
		std::cerr << "Unable to convert utf8 string " << str 
			<< " into file name encoding.";
		if(err)
			std::cerr << " Error: " << err->message << ".";
		std::cerr << std::endl;
		return false;
	}
	out = get_impl(gstr);
	return true;
}

class Main
{
public:
	Main(void)
	{
		buffer.reserve(buffer_size);
	}
	void Convert(int argc, char * argv [])
	{
		ParseCommandLine(argc, argv);
		rdic_fh.reset(fopen(RdicFileName.c_str(), "rb"));
		if(!rdic_fh) {
			std::cerr << "Unable to open file " << RdicFileName
				<< " file for reading" << std::endl;
			throw EMain();
		}
		if(!ProcessIndexFile())
			throw EMain();
	}
private:
	void ParseCommandLine(int argc, char * argv [])
	{
		glib::CharStr res_dir_name;
		glib::CharStr rifo_file_name;
		static GOptionEntry entries[] = {
			{ "res-dir", 'd', 0, G_OPTION_ARG_FILENAME, get_addr(res_dir_name),
				"resource directory", 
				"dir_name"},
			{ "res-info-file", 'i', 0, G_OPTION_ARG_FILENAME, get_addr(rifo_file_name),
				"Stardict resource file with .rifo extension", 
				"file_name"},
			{ NULL },
		};
		glib::OptionContext opt_cnt(g_option_context_new(""));
		g_option_context_add_main_entries(get_impl(opt_cnt), entries, NULL);
		g_option_context_set_help_enabled(get_impl(opt_cnt), TRUE);
		g_option_context_set_summary(get_impl(opt_cnt), 
			"Convert Stardict resource database file into resource directory.\n"
			"\n"
			"Note. If you see multiple encoding conversion errors, it is probably because\n"
			"your file system encoding was not recognized correctly. Set G_FILENAME_ENCODING\n"
			"environment variable to specify encoding. Another possibility is that your\n"
			"file system encoding is not able to represent all required unicode characters.\n"
			);
		glib::Error err;
		if (!g_option_context_parse(get_impl(opt_cnt), &argc, &argv, get_addr(err))) {
			std::cerr << "Options parsing failed: " <<  err->message << std::endl;
			throw EMain();
		}
		if(res_dir_name) {
			ResDir = get_impl(res_dir_name);
			size_t len = ResDir.length();
			if(len > 0 && ResDir[len-1] == G_DIR_SEPARATOR)
				ResDir.resize(len-1);
		} else {
			std::cerr << "You did not specify resource directory to save extracted files. "
			<< "Use --res-dir option." << std::endl;
			throw EMain();
		}
		std::string ResDbBase;
		if(rifo_file_name) {
			ResDbBase = get_impl(rifo_file_name);
			size_t len = ResDbBase.length();
			size_t ext_len = sizeof(".rifo") - 1;
			if(len <= ext_len || ResDbBase.compare(len-ext_len, ext_len, ".rifo") != 0) {
				std::cerr << "Resource database file must have .rifo extension" 
									<< std::endl; 
				throw EMain();
			}
			// erase extension
			ResDbBase.erase(len-ext_len);
		} else {
			std::cerr << "You did not specify resource dictionary file to read. "
			<< "Use --res-info-file option" << std::endl;
			throw EMain();
		}
		RdicFileName = ResDbBase + ".rdic";
		RidxFileName = ResDbBase + ".ridx";
		RifoFileName = ResDbBase + ".rifo";
	}
	/* Read the index file and extract all files listed there. */
	bool ProcessIndexFile(void)
	{
		ridx_fh.reset(fopen(RidxFileName.c_str(), "rb"));
		if(!ridx_fh) {
			std::cerr << "Unable to open file for reading: " << RidxFileName
				<< std::endl;
			return false;
		}
		TIndexEntity IndexEntity;
		while(ReadIndexEntity(IndexEntity)) {
			if(!Utf8ToFileName(IndexEntity.db_file_name, IndexEntity.fs_file_name))
				continue;
			IndexEntity.fs_file_name = dir_separator_db_to_fs(IndexEntity.fs_file_name);
			ExtractResourceFile(IndexEntity);
		}
		return true;
	}
	bool ReadIndexEntity(TIndexEntity& IndexEntity)
	{
		// read file name
		std::string &res = IndexEntity.db_file_name;
		res.clear();
		res.reserve(256);
		int c;
		while(true) {
			c = fgetc(get_impl(ridx_fh));
			if(c == EOF) {
				if(res.empty())
					return false; // all entities are read
				std::cerr << "Unexpected end of index file" << std::endl;
				throw EMain();
			}
			if(!c)
				break;
			res += (char)c;
		}
		
		guint32 t;
		// read offset
		if(sizeof(t) != fread(&t, 1, sizeof(t), get_impl(ridx_fh))) {
			std::cerr << "Unexpected end of index file" << std::endl;
			throw EMain();
		}
		IndexEntity.offset = g_ntohl(t);
		// read size
		if(sizeof(t) != fread(&t, 1, sizeof(t), get_impl(ridx_fh))) {
			std::cerr << "Unexpected end of index file" << std::endl;
			throw EMain();
		}
		IndexEntity.size = g_ntohl(t);
		
		return true;
	}
	bool ExtractResourceFile(TIndexEntity& IndexEntity)
	{
		std::string full_file_name = ResDir + G_DIR_SEPARATOR 
			+ IndexEntity.fs_file_name;
		
		// create intermediate directories
		std::string dir_name;
		size_t pos = full_file_name.find_last_of(G_DIR_SEPARATOR);
		if(pos != std::string::npos)
			dir_name = full_file_name.substr(0, pos);
		if(!dir_name.empty())
			if(g_mkdir_with_parents(dir_name.c_str(), 0750)) {
				std::cerr << "Unable to create directory " << dir_name << std::endl;
				return false;
			}
		
		// copy file
		clib::File res_fh(fopen(full_file_name.c_str(), "wb"));
		if(!res_fh) {
			std::cerr << "Unable to open file for writing: " << full_file_name
				<< std::endl;
			return false;
		}
		if(fseek(get_impl(rdic_fh), IndexEntity.offset, SEEK_SET)) {
			std::cerr << "Problem accessing " << RdicFileName << " resource file, "
			<< "maybe incorrect file offset." << std::endl;
			return false;
		}
		guint32 to_read_total = IndexEntity.size; // bytes to read total
		guint32 to_read; // bytes to read in one iteration
		size_t read_bytes;
		while(to_read_total > 0) {
			to_read = buffer_size < to_read_total ? buffer_size : to_read_total;
			read_bytes = fread(&buffer[0], 1, to_read, get_impl(rdic_fh));
			if(ferror(get_impl(rdic_fh))) {
				std::cerr << "Error while reading file " << RdicFileName
					<< ". Unrecoverable error " << std::endl;
				throw EMain();
			}
			if(read_bytes != to_read) {
				std::cerr << "File " << IndexEntity.db_file_name << " cannot be read "
				<< "completely, maybe incorrect file size in database." << std::endl;
				return false;
			}
			if(read_bytes != fwrite(&buffer[0], 1, read_bytes, get_impl(res_fh))) {
				std::cerr << "Error writing into " << full_file_name << " file." 
				<< std::endl;
				return false;
			}
			to_read_total -= read_bytes;
		}
		return true;
	}
private:
	/* size of the buffer used to copy file contents into resource database. */
	static const size_t buffer_size = 1024*1024;
	std::vector<char> buffer;
	clib::File rdic_fh;
	clib::File ridx_fh;
	// resource directory without terminating dir separator char
	std::string ResDir;
	std::string RdicFileName;
	std::string RidxFileName;
	std::string RifoFileName;
};

int
main(int argc, char * argv [])
{
	setlocale(LC_ALL, "");
	Main m;
	try {
		m.Convert(argc, argv);
		std::cout << "Done." << std::endl;
	} catch(EMain&) {
		return 1;
	}
	return 0;
}
