#ifndef LIB_RES_STORE_H_
#define LIB_RES_STORE_H_

#include <string>
#include "libcommon.h"

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

typedef std::vector<resitem_t> resitem_vect_t;


class resource_storage: public i_resource_storage
{
public:
	resource_storage(void);
	~resource_storage(void);
	TLoadResult load(const std::string& dirname, print_info_t print_info);
	// filename uses database directory separator
	bool have_file(const std::string& filename) const;
private:
	void clear(void);
	resource_database *db;
	resource_files *files;
};

#endif /* LIB_RES_STORE_H_ */
