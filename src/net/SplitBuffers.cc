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

	// if "needle" is not found, append "extra" at the end.
	// i.e. m_top will remain unchanged, and "follow" in the new
	// segment will be empty
	m_segs.emplace_back(
		std::move(extra),
		offset != m_top.npos ?
			m_top.substr(offset + (opt == Option::replace ? needle.size() : 0)) :
			std::string_view{}
	);

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
