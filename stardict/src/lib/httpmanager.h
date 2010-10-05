#ifndef _STARDICT_HTTP_MANAGER_H_
#define _STARDICT_HTTP_MANAGER_H_

#include "http_client.h"

class HttpManager {
public:
	HttpManager();
	~HttpManager();
	void SendHttpGetRequest(const char* shost, const char* sfile, gpointer userdata);
	void SendHttpGetRequestWithCallback(const char* shost, const char* sfile, get_http_response_func_t callback_func, gpointer userdata);
	void Remove(HttpClient *http_client);
private:
	std::list<HttpClient *> client_list;
};

#endif
