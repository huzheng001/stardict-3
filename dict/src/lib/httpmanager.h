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

#ifndef _STARDICT_HTTP_MANAGER_H_
#define _STARDICT_HTTP_MANAGER_H_

#include "http_client.h"

class HttpManager {
public:
	HttpManager();
	~HttpManager();
	void Add(HttpClient* client);
	void Remove(HttpClient *http_client);
private:
	std::list<HttpClient *> client_list;
};

#endif
