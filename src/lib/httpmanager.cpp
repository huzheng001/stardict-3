#include "httpmanager.h"

HttpManager::HttpManager()
{
}

HttpManager::~HttpManager()
{
	for (std::list<HttpClient *>::iterator i = client_list.begin(); i != client_list.end(); ++i) {
		delete *i;
	}
}

void HttpManager::SendHttpGetRequest(const char* shost, const char* sfile, gint userdata)
{
	HttpClient *client = new HttpClient();
	client_list.push_back(client);
	client->SendHttpGetRequest(shost, sfile, userdata);
}

void HttpManager::Remove(HttpClient *http_client)
{
	client_list.remove(http_client);
	delete http_client;
}
