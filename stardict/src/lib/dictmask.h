#ifndef _STARDICT_DICTMASK_H_
#define _STARDICT_DICTMASK_H_

enum InstantDictType {
	InstantDictType_UNKNOWN = 0,
	InstantDictType_LOCAL,
	InstantDictType_VIRTUAL,
	InstantDictType_NET,
};

struct InstantDictIndex {
	InstantDictType type;
	size_t index;
};

#endif
