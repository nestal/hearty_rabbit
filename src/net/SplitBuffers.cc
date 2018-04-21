/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 4/22/18.
//

#include "SplitBuffers.hh"

namespace hrb {

void SplitBuffers::value_type::extra(std::string_view needle, std::string&& extra, Option opt)
{
	auto offset = needle.empty() ? m_top.npos : m_top.find(needle);

	// for Option::replace: the needle will not be included in "m_top" and "follow".
	// for Option::inject_after: the needle will be included in "m_top"
	// for Option::inject_middle: the first half of the needle will be included in "m_top"
	// and the second half will be included in "follow".

	// It's common for the needle to be an empty HTML tag and we want to inject something
	// inside the tag using Option::inject_middle, e.g. needle is "<script></script>
	// and "extra" will be the javascript. In this case, the length of the needle is
	// odd (because of the extra / character). Here we will put the first n/2 bytes
	// to "m_top" and the rest n/2+1 bytes to follow. Note that in C/C++ division will
	// always truncate the remainder.

	std::size_t needle_top{}, needle_follow{};
	switch (opt)
	{
		// input:  <before><needle><after>
		// output: <before><extra><after>
		case Option::replace:       needle_top = 0;             needle_follow = 0; break;

		// input:  <before><needle><after>
		// output: <before><extra><needle><after>
		case Option::inject_before: needle_top = 0;             needle_follow = needle.size(); break;

		// input:  <before><needle><after>
		// output: <before><needle><extra><after>
		case Option::inject_after:  needle_top = needle.size(); needle_follow = 0; break;

		// input:  <before><needle><after>
		// output: <before><nee<extra>dle><after>
		case Option::inject_middle:
			needle_top    = needle.size()/2;
			needle_follow = needle.size()-needle_top;
			break;
	}

	// offset points to the position of the needle
	// <before><needle><after>
	//         ^
	//         offset

	// if "needle" is not found, append "extra" at the end.
	// i.e. m_top will remain unchanged, and "follow" in the new
	// segment will be empty
	std::string_view follow;
	if (offset != m_top.npos)
		follow = m_top.substr(offset + needle.size() - needle_follow);

	m_segs.emplace_back(std::move(extra), follow);

	if (offset != m_top.npos)
		offset += needle_top;

	// substr() works even when offset is npos
	// if use remove_suffix(), need to check against npos ourselves
	// less code -> fewer bugs
	m_top = m_top.substr(0, offset);
}

SplitBuffers::const_buffers_type SplitBuffers::value_type::data() const
{
	const_buffers_type result{{m_top.data(), m_top.size()}};

	for (auto&& seg : m_segs)
	{
		result.emplace_back(seg.extra.data(), seg.extra.size());
		result.emplace_back(seg.follow.data(), seg.follow.size());
	}

	return result;
}

} // end of namespace
