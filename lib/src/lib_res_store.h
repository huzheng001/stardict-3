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

#ifndef LIB_RES_STORE_H_
#define LIB_RES_STORE_H_

#include <string>
#include "libcommon.h"
#include "lib_dict_verify.h"

class i_resource_storage {
public:
	virtual bool have_file(const std::string& filename) const = 0;
};

class resource_database;
class resource_files;

struct resitem_t {
	std::string type;
	std::string key;
};

enum StorageType {
	StorageType_UNKNOWN,
	// files in res directory
	StorageType_FILE,
	// database consisting of files: res.rifo, res.ridx, res.rdic
	StorageType_DATABASE
};

typedef std::vector<resitem_t> resitem_vect_t;


class resource_storage: public i_resource_storage
{
public:
	resource_storage(void);
	~resource_storage(void);
	TLoadResult load(const std::string& dirname);
	// filename uses database directory separator
	bool have_file(const std::string& filename) const;
	VerifResult get_verif_result(void) const { return verif_result; }
	StorageType get_storage_type(void) const;
	/* true if res.ridx.gz used, res.ridx otherwise
	 * only when get_storage_type == StorageType_DATABASE */
	bool res_ridx_compressed(void) const;
	/* true if res.rdic.dz used, res.rdic otherwise
	 * only when get_storage_type == StorageType_DATABASE */
	bool res_rdic_compressed(void) const;
private:
	void clear(void);
	resource_database *db;
	resource_files *files;
	VerifResult verif_result;
};

#endif /* LIB_RES_STORE_H_ */
