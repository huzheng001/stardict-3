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

#ifndef _COMPOSITE_LOOKUP_H_
#define _COMPOSITE_LOOKUP_H_

#include <string>
#include <list>

/* A lookup may consist of a number of requests: to local dictionaries, 
to net dictionaries, to StarDict net dictionary.
This class tracks all requests making up one lookup. 
This class can tell when all responses are got. */
class CompositeLookup
{
public:
	CompositeLookup(void);
	void new_lookup(void);
	void done_lookup(void);
	bool is_got_all_responses(void) const;
	void send_net_dict_request(const std::string& dict_id, const std::string& key);
	bool got_net_dict_responce(const std::string& dict_id, const std::string& key);
	void send_StarDict_net_request(unsigned int seq);
	bool got_StarDict_net_responce(unsigned int seq);
private:
	struct NetDictRequest
	{
		NetDictRequest(const std::string& dict_id, const std::string& key)
			: dict_id(dict_id), key(key)
		{
		}
		bool operator==(const NetDictRequest& right) const
		{
			return dict_id == right.dict_id && key == right.key;
		}
		std::string dict_id;
		std::string key;
	};
	/* when true the lookup is in process of creation. We do not know all the requests 
	that will make up the lookup. */
	bool BuildingLookup;
	typedef std::list<NetDictRequest> NetDictRequestsList;
	/* all requests sent to net dictionaries that have not got responses yet */
	NetDictRequestsList NetDictRequests;
	/* expected seq of the StarDict net response, 0 - nothing is expected */
	unsigned int StarDictNetSeq;
};

#endif
