#include <algorithm>
#include <glib.h>
#include "compositelookup.h"

CompositeLookup::CompositeLookup(void)
:
BuildingLookup(false),
StarDictNetSeq(0)
{
}

void CompositeLookup::new_lookup(void)
{
	BuildingLookup = true;
	NetDictRequests.clear();
	StarDictNetSeq = 0;
}

void CompositeLookup::done_lookup(void)
{
	BuildingLookup = false;
}

bool CompositeLookup::is_got_all_responses(void) const
{
	if(BuildingLookup)
		return false;
	return StarDictNetSeq == 0 && NetDictRequests.empty();
}

void CompositeLookup::send_net_dict_request(const std::string& dict_id, const std::string& key)
{
	NetDictRequest request(dict_id, key);
	g_assert(NetDictRequests.end() == std::find(NetDictRequests.begin(), NetDictRequests.end(), request));
	NetDictRequests.push_back(request);
}

/* returns true if got expected responce */
bool CompositeLookup::got_net_dict_responce(const std::string& dict_id, const std::string& key)
{
	NetDictRequest response(dict_id, key);
	NetDictRequestsList::iterator it = std::find(NetDictRequests.begin(), NetDictRequests.end(), response);
	if(it != NetDictRequests.end()) {
		NetDictRequests.erase(it);
		return true;
	}
	return false;
}

void CompositeLookup::send_StarDict_net_request(unsigned int seq)
{
	StarDictNetSeq = seq;
}

/* returns true if got expected responce */
bool CompositeLookup::got_StarDict_net_responce(unsigned int seq)
{
	if(StarDictNetSeq == 0)
		return false;
	if(seq == StarDictNetSeq) {
		StarDictNetSeq = 0;
		return true;
	}
	return false;
}
