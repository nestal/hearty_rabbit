/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 4/22/18.
//

#include "StringTemplate.hh"

#include <cassert>

namespace hrb {

void StringTemplate::replace(std::string_view needle)
{
	inject(needle, 0, 0);
}

void StringTemplate::replace(std::string_view needle, std::string_view subneedle)
{
	if (auto offset = needle.find(subneedle); offset != needle.npos)
		inject(needle, offset, needle.size() - offset - subneedle.size());
}

void StringTemplate::inject_before(std::string_view needle)
{
	inject(needle, 0, needle.size());
}

void StringTemplate::inject_after(std::string_view needle)
{
	inject(needle, needle.size(), 0);
}

void StringTemplate::inject(
	std::string_view needle,
	std::size_t needle_before,
	std::size_t needle_after
)
{
	assert(!m_src.empty());

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

	for (auto i = 0UL; i < m_src.size(); i++)
	{
		auto offset = needle.empty() ? m_src[i].npos : m_src[i].find(needle);

		// offset points to the position of the needle
		// <before><needle><after>
		//         ^
		//         offset

		// if "needle" is not found, append "extra" at the end.
		// i.e. m_top will remain unchanged, and "follow" in the new
		// segment will be empty
		if (offset != m_src[i].npos)
		{
			auto before = m_src[i].substr(0, offset + needle_before);
			auto after  = m_src[i].substr(offset + needle.size() - needle_after);

			// WARNING! modifying the vector when looping it
			// That is why we always use indexes to access the vector
			m_src[i] = before;
			m_src.emplace(m_src.begin() + i + 1, after);
			return;
		}
	}

	// special handling for not found
	m_src.emplace_back();
}

} // end of namespace hrb
