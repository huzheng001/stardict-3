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

void HttpManager::SendHttpGetRequest(const char* shost, const char* sfile, gpointer userdata)
{
	HttpClient *client = new HttpClient();
	client_list.push_back(client);
	client->SendHttpGetRequest(shost, sfile, userdata);
}

void HttpManager::SendHttpGetRequestWithCallback(const char* shost, const char* sfile, get_http_response_func_t callback_func, gpointer userdata)
{
	HttpClient *client = new HttpClient();
	client_list.push_back(client);
	client->SendHttpGetRequestWithCallback(shost, sfile, callback_func, userdata);
}

void HttpManager::Remove(HttpClient *http_client)
{
	client_list.remove(http_client);
	delete http_client;
}
