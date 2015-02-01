/*
 * This file is part of makedict - convertor from any
 * dictionary format to any http://xdxf.sourceforge.net
 *
 * Copyright (C) Evgeniy Dushistov, 2005-2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

//#define DEBUG
#ifdef DEBUG
#  include "file.h"
#endif

#include <deque>

#include "utils.h"

#include "normalize_tags.h"


#ifdef DEBUG
static void print_sections(const std::string& banner,
			   std::vector<Section>::const_iterator p,
			   std::vector<Section>::const_iterator end)
{
	StdOut << banner << "\n";
	for (; p!=end; ++p)
		StdOut << p->begin.info->open_sig << " : " << p->begin.p
			  << " : " << p->end
			  << "\n";
}
#endif

void NormalizeTags::add_section(const Section& sec)
{
	std::vector<Section>::reverse_iterator rp=sections.rbegin();
	while (rp!=sections.rend() && *rp==sec)
		++rp;
	sections.insert(rp.base(), sec);
}

bool NormalizeTags::add_open_tag(std::string &resstr, const char *&p)
{
	TagInfoList::iterator i =
		find_if(taginfo_list.begin(), taginfo_list.end(),
			bind2nd(std::mem_fun_ref<bool, TagInfo, const char*>(&TagInfo::begin_from_this), p));
	if (i != taginfo_list.end()) {
		if (!i->have_value) {
			open_tags.push_back(Tag(i, resstr.length(), cur_timestamp_++));
			p += i->open_sig.length();
		} else {
			p += i->open_sig.length();
			std::string val;
			tag_value(p, val);

			if (!val.empty())
				open_tags.push_back(Tag(i, resstr.length(), val, cur_timestamp_++));
			else
				open_tags.push_back(Tag(i, resstr.length(), cur_timestamp_++));
		}
		return true;
	} else
		return false;
}

bool NormalizeTags::add_close_tag(std::string &resstr, const char *&p)
{
	TagInfoList::iterator i =
		find_if(taginfo_list.begin(), taginfo_list.end(),
			bind2nd(std::mem_fun_ref<bool, TagInfo, const char*>(&TagInfo::end_on_this), p));
	if (i != taginfo_list.end()) {
		TagStack::reverse_iterator openi =
			find_if(open_tags.rbegin(), open_tags.rend(),
				bind2nd(std::mem_fun_ref<bool, Tag, int>(&Tag::have_such_code), i->code));

		if (openi != open_tags.rend()) {
			add_section(Section(openi, resstr.length()));
			open_tags.erase((++openi).base());
		}
		p += i->close_sig.length();
		return true;
	} else
		return false;
}

void NormalizeTags::operator()(std::string& resstr, std::string& datastr)
{
#ifdef DEBUG
	print_sections("before all, sections: ", sections.begin(), sections.end());
#endif
	for (std::vector<Tag>::reverse_iterator ri = open_tags.rbegin();
	     ri != open_tags.rend(); ++ri)
		add_section(Section(ri, resstr.length()));
	/*
	 * algorithm of dsl correction is simple:
	 * we have a container with "good" sections,
	 * I mean sections with such property:
	 * if two of them have a common part then
	 * one of these sections is a part of another
	 */
	std::vector<Section> good_sections;
	std::vector<Section>::size_type p_pos, q_pos;
	for (std::vector<Section>::iterator p = sections.begin(),
		     end = sections.end(); p != end; ++p) {
		p_pos=p-sections.begin();
		//and we have container that holds all sections
		//sorted using "<" of struct Section

		std::sort(p, end);

		p=sections.begin()+p_pos;
		std::string::size_type end_pos = p->end;
		//first section is right
		good_sections.push_back(*p);

		//other sections we divide on two parts if the have common part
		//with "good" section
		for (std::vector<Section>::iterator q = p + 1;
		     q != end && q->begin.p < end_pos; ++q)
			if (q->end>end_pos) {
				Tag t(q->begin);
				t.p=end_pos;

				p_pos=p-sections.begin();
				q_pos=q-sections.begin();
				sections.push_back(Section(t, q->end));
				q=sections.begin()+q_pos;
				q->end=end_pos;
				end=sections.end();
				p=sections.begin()+p_pos;
			}
	}


	datastr.resize(0);
	std::deque<Tag> close_tags;
	std::string::size_type cur_pos=0;
	for (std::vector<Section>::iterator p = good_sections.begin();
	     p != good_sections.end(); ++p) {
		while (!close_tags.empty() && cur_pos <= close_tags.front().p &&
		       close_tags.front().p <= p->begin.p) {
			datastr.append(resstr, cur_pos,
				       close_tags.front().p-cur_pos);
			datastr+=close_tags.front().info->close_val;
			cur_pos+=close_tags.front().p-cur_pos;
			close_tags.pop_front();
		}

		if (cur_pos<=p->begin.p) {
			datastr.append(resstr, cur_pos, p->begin.p-cur_pos);
			cur_pos+=p->begin.p-cur_pos;
		}


		datastr+=p->begin.info->open_val;

		if (p->begin.info->have_value && !p->begin.value.empty()) {
			switch (p->begin.info->code) {
			case TagInfo::tColor:
			{
				datastr.erase(datastr.length()-1);
				std::string tag = " c=\""+p->begin.value+"\">";
				datastr+=tag;
				break;
			}
			default:
				/*nothing*/;
			}
		}
		p->begin.p=p->end;
		close_tags.push_front(p->begin);
	}

	while (!close_tags.empty()) {
		datastr.append(resstr, cur_pos, close_tags.front().p-cur_pos);
		datastr+=close_tags.front().info->close_val;
		cur_pos+=close_tags.front().p-cur_pos;
		close_tags.pop_front();
	}
	if (cur_pos<resstr.length())
		datastr.append(resstr, cur_pos, resstr.length()-cur_pos);
#ifdef DEBUG
	StdOut<<datastr<<"\n";
#endif
}

void tag_value(const char *&p, std::string& val)
{
  const char *closed_braket=strchr(p, ']');
  if (closed_braket!=NULL) {
    const char *q=closed_braket-1;

    while (p!=q && (*p==' ' || *p=='\t'))
      ++p;
    while (q!=p && (*q==' ' || *q=='\t'))
      --q;

    if (q>p) {
      val.assign(p, q+1-p);
      ::tolower(val);
    }
    p=closed_braket+1;
  }
}

