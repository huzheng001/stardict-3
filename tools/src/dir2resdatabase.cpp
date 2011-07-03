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
#include <algorithm>

#include <glib.h>
#include <glib/gstdio.h>
#include "libcommon.h"

class EMain {};

/* convert string from file name encoding to utf8 */
bool FileNameToUtf8(const std::string& str, std::string& out)
{
	size_t len = str.length();
	gsize bytes_read, bytes_written;
	glib::Error err;
	glib::CharStr gstr(g_filename_to_utf8(str.c_str(), len, &bytes_read, 
		&bytes_written, get_addr(err)));
	if(!gstr || bytes_read != len) {
		std::cerr << "Unable to convert string " << str << " into utf-8 encoding.";
		if(err)
			std::cerr << " Error: " << err->message << ".";
		std::cerr << std::endl;
		return false;
	}
	out = get_impl(gstr);
	return true;
}

struct TFileEntity
{
	TFileEntity(void)
	:
		size(0),
		offset(0)
	{
		
	}
	
	// file name in the file system, in file name encoding
	std::string fs_file_name;
	// file name to store in resource database, in utf8 encoding
	std::string db_file_name;
	// file size in bytes
	size_t size;
	// file offset in the resource database
	size_t offset;
};

// this function is used to sort file entities
bool FileEntitySort(const TFileEntity& v1, const TFileEntity& v2)
{
	// Stardict requires this function be used to sort index words
	return strcmp(v1.db_file_name.c_str(), v2.db_file_name.c_str()) < 0;
}


class Main
{
public:
	Main(void)
	:
		op_verbose(FALSE)
	{
		buffer.reserve(buffer_size);
	}
	void Convert(int argc, char * argv [])
	{
		ParseCommandLine(argc, argv);
		rdic_fh.reset(fopen(RdicFileName.c_str(), "wb"));
		if(!rdic_fh) {
			std::cerr << "Unable to open file " << RdicFileName
				<< " file for writing" << std::endl;
			throw EMain();
		}
		bool success = ProcessDir(ResDir);
		rdic_fh.reset(NULL);
		if(!success)
			throw EMain();
		SortFileEntityList();
		if(!WriteIndexFile())
			throw EMain();
		if(!WriteInfoFile())
			throw EMain();
	}
	
private:
	void ParseCommandLine(int argc, char * argv [])
	{
		glib::CharStr res_dir_name;
		glib::CharStr rifo_file_name;
		static GOptionEntry entries[] = {
			{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &op_verbose,
				"print additional debugging information", NULL },
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
			"Convert resource directory into Stardict resource database file.\n"
			"\n"
			"Note. If you see multiple encoding conversion errors, it is probably because\n"
			"your file system encoding was not recognized correctly. Set G_FILENAME_ENCODING\n"
			"environment variable to specify encoding.\n"
			);
		glib::Error err;
		if (!g_option_context_parse(get_impl(opt_cnt), &argc, &argv, get_addr(err))) {
			std::cerr << "Options parsing failed: " <<  err->message << std::endl;
			throw EMain();
		}
		if(res_dir_name)
			ResDir = get_impl(res_dir_name);
		else {
			std::cerr << "You did not specify resource directory to process. "
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
			std::cerr << "You did not specify resource dictionary file to create. "
			<< "Use --res-info-file option" << std::endl;
			throw EMain();
		}
		RdicFileName = ResDbBase + ".rdic";
		RidxFileName = ResDbBase + ".ridx";
		RifoFileName = ResDbBase + ".rifo";
	}
	/* Process the directory specified, build FileEntityList list, fill .rdic 
	 * file. base_dir in file name encoding */
	bool ProcessDir(const std::string& base_dir)
	{
		if(base_dir.empty())
			return false;
		/* directories to process
		 * Each directory name is a string terminated with not-G_DIR_SEPARATOR char.
		 * For base_dir we explicitly check that condition. */
		std::stack<std::string> dirs;
		if(base_dir[base_dir.length()-1] == G_DIR_SEPARATOR)
			dirs.push(base_dir.substr(0, base_dir.length()-1));
		else
			dirs.push(base_dir);
		/* How many characters have to be casted away from full_file_name to get 
		 * a file name that will be stored in resource database. */
		const size_t base_dir_length = dirs.top().length() + 1;
		std::string dir_name;
		std::string full_file_name;
		while(!dirs.empty()) {
			dir_name = dirs.top();
			dirs.pop();
			
			glib::Error err;
			glib::Dir gdir(g_dir_open(dir_name.c_str(), 0, get_addr(err)));
			if(!gdir) {
				std::cerr << "Unable to open " << dir_name << " directory: " 
					<< err->message << std::endl;
				continue;
			}
			
			const gchar* file_name = NULL;
			while((file_name = g_dir_read_name(get_impl(gdir)))) {
				full_file_name = dir_name + G_DIR_SEPARATOR + file_name;
				if(g_file_test(full_file_name.c_str(), G_FILE_TEST_IS_REGULAR)) {
					if(op_verbose) {
						std::cout << "Adding file: " << full_file_name << "... ";
						std::cout.flush();
					}
					if(AddFile(base_dir_length, full_file_name))
						if(op_verbose)
							std::cout << "done" << std::endl;
				} else if(g_file_test(full_file_name.c_str(), G_FILE_TEST_IS_DIR)) {
					dirs.push(full_file_name);
				} else
					std::cerr << "Unknown file type " << full_file_name << ". Skipping." 
						<< std::endl;
			}
		}
		std::cout.flush();
		std::cerr.flush();
		return true;
	}
	bool AddFile(size_t base_dir_length, const std::string& full_file_name)
	{
		TFileEntity FileEntity;
		FileEntity.fs_file_name = full_file_name;
		if(!FileNameToUtf8(FileEntity.fs_file_name.substr(base_dir_length), 
			FileEntity.db_file_name))
			return false;
		FileEntity.db_file_name = dir_separator_fs_to_db(FileEntity.db_file_name);
		if(!AddFileToRdic(FileEntity))
			return false;
		FileEntityList.push_back(FileEntity);
		return true;
	}
	bool AddFileToRdic(TFileEntity& FileEntity)
	{
		clib::File fh(fopen(FileEntity.fs_file_name.c_str(), "rb"));
		if(!fh) {
			std::cerr << "Unable to open file for reading: " << FileEntity.fs_file_name
				<< std::endl;
			return false;
		}
		FileEntity.offset = ftell(get_impl(rdic_fh));
		size_t read_bytes;
		while(true) {
			read_bytes = fread(&buffer[0], 1, buffer_size, get_impl(fh));
			if(ferror(get_impl(fh))) {
				std::cerr << "Error while reading file " << FileEntity.fs_file_name
					<< ". Unrecoverable error " << std::endl;
				throw EMain();
			}
			if(read_bytes == 0)
				break; // file is done
			if(read_bytes != fwrite(&buffer[0], 1, read_bytes, get_impl(rdic_fh))) {
				std::cerr << "Error writing into " << RdicFileName 
					<< " file. Unrecoverable error " << std::endl;
				throw EMain();
			}
		}
		FileEntity.size = ftell(get_impl(rdic_fh)) - FileEntity.offset;
		return true;
	}
	void SortFileEntityList(void)
	{
		std::sort(FileEntityList.begin(), FileEntityList.end(), FileEntitySort);
	}
	bool WriteIndexFile(void)
	{
		clib::File fh(fopen(RidxFileName.c_str(), "wb"));
		if(!fh) {
			std::cerr << "Unable to open file for writing: " << RidxFileName
				<< std::endl;
			return false;
		}
		guint32 t;
		for(TFileEntityList::iterator i = FileEntityList.begin(); 
			i != FileEntityList.end(); ++i) {
			// file name with terminating '\0' char
			fwrite(i->db_file_name.c_str(), i->db_file_name.length() + 1, 1, 
				get_impl(fh));
			// offset
			t = g_htonl(i->offset);
			fwrite(&t, 4, 1, get_impl(fh));
			// size
			t = g_htonl(i->size);
			fwrite(&t, sizeof(t), 1, get_impl(fh));
		}
		IndexFileSize = ftell(get_impl(fh));
		return true;
	}
	bool WriteInfoFile(void)
	{
		std::ostringstream str;
		str << "StarDict's storage ifo file\n"
			<< "version=3.0.0\n"
			<< "filecount=" << FileEntityList.size() << "\n"
			<< "ridxfilesize=" << IndexFileSize << "\n";
		clib::File fh(fopen(RifoFileName.c_str(), "wb"));
		if(!fh) {
			std::cerr << "Unable to open file for writing: " << RifoFileName 
				<< std::endl;
			return false;
		}
		fwrite(str.str().c_str(), str.str().length(), 1, get_impl(fh));
		return true;
	}
private:
	/* size of the buffer used to copy file contents into resource database. */
	static const size_t buffer_size = 1024*1024;
	std::vector<char> buffer;
	
	clib::File rdic_fh;
	std::string ResDir;
	std::string RdicFileName;
	std::string RidxFileName;
	std::string RifoFileName;
	typedef std::vector<TFileEntity> TFileEntityList;
	TFileEntityList FileEntityList;
	size_t IndexFileSize;
	gboolean op_verbose;
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
