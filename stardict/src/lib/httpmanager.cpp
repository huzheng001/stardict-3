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

void HttpManager::Add(HttpClient* client)
{
	client_list.push_back(client);
}

void HttpManager::Remove(HttpClient *http_client)
{
	client_list.remove(http_client);
	delete http_client;
}
