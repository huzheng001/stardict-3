#ifndef _STARDICT_DICTMASK_H_
#define _STARDICT_DICTMASK_H_

enum InstantDictType {
	InstantDictType_UNKNOWN,
	InstantDictType_LOCAL,
	InstantDictType_VIRTUAL,
};

struct InstantDictIndex {
	InstantDictType type;
	size_t index;
};

#endif
