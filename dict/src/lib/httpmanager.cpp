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
