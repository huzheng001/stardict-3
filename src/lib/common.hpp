#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <glib.h>
#include <list>
#include <string>

enum DictInfoType {
	DictInfoType_NormDict,
	DictInfoType_TreeDict,
	DictInfoType_ResDb
};

// This structure contains all information about dictionary or Resource Storage
// database.
struct DictInfo {
  std::string ifo_file_name;
  union {
  guint32 wordcount;
  guint32 filecount;
  };
  guint32 synwordcount;
  std::string bookname;
  std::string author;
  std::string email;
  std::string website;
  std::string date;
  std::string description;
  guint32 index_file_size;
  std::string sametypesequence;
  std::string dicttype;
  std::string version;
  DictInfo(void);
  bool load_from_ifo_file(const std::string& ifofilename, DictInfoType infotype);
  void clear(void);
};

#endif//!_COMMON_HPP_
