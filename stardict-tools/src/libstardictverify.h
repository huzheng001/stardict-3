#ifndef _LIBSTARDICTVERIFY_H_
#define _LIBSTARDICTVERIFY_H_

#include <vector>
#include "libcommon.h"

extern int stardict_verify(const char *ifofilename, print_info_t print_info);

struct region_t {
	guint32 offset;
	guint32 size;
};

template <class item_t>
void verify_data_blocks_overlapping(std::vector<item_t*>& sort_index,
	std::vector<std::pair<size_t, size_t> >& overlapping_blocks)
{
	for(size_t i=0; i<sort_index.size(); ++i) {
		for(size_t j=i+1; j<sort_index.size()
			&& sort_index[i]->offset + sort_index[i]->size > sort_index[j]->offset; ++j) {
			if(sort_index[i]->offset == sort_index[j]->offset
				&& sort_index[i]->size == sort_index[j]->size)
				continue;
			overlapping_blocks.push_back(std::pair<size_t, size_t>(i, j));
		}
	}
}

template <class item_t>
void verify_unused_regions(std::vector<item_t*>& sort_index,
		std::vector<region_t>& unused_regions, guint32 filesize)
{
	region_t region;
	guint32 low_boundary=0;
	for(size_t i=0; i<sort_index.size(); ++i) {
		const guint32 l_left = sort_index[i]->offset;
		const guint32 l_right = sort_index[i]->offset + sort_index[i]->size;
		if(l_left < low_boundary) {
			if(l_right > low_boundary)
				low_boundary = l_right;
		} if(l_left == low_boundary) {
			low_boundary = l_right;
		} else { // gap found
			region.offset = low_boundary;
			region.size = l_left - low_boundary;
			unused_regions.push_back(region);
			low_boundary = l_right;
		}
	}
	if(low_boundary < filesize) {
		region.offset = low_boundary;
		region.size = filesize - low_boundary;
		unused_regions.push_back(region);
	}
}

#endif

