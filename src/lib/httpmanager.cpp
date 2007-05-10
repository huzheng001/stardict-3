#include "httpmanager.h"

HttpManager::HttpManager()
{
}

HttpManager::~HttpManager()
{
	for (std::list<HttpClient *>::iterator i = client_list.begin(); i != client_list.end(); ++i) {
		g_free((*i)->buffer);
		delete *i;
	}
}

void HttpManager::SendHttpGetRequest(const char* shost, const char* sfile)
{
	HttpClient *client = new HttpClient();
	client->SendHttpGetRequest(shost, sfile);
	client_list.push_back(client);
}
