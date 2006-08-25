#ifndef _DICT_CLINET_HPP_
#define _DICT_CLINET_HPP_

#include <glib.h>

class DictClient {
public:
	bool connect(const char *host, int port);
private:

};

#endif//!_DICT_CLINET_HPP_
