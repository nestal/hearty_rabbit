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

#include <boost/range/adaptor/reversed.hpp>

namespace hrb {

void SplitBuffers::value_type::replace(std::string_view needle, std::string&& extra)
{
	inject(needle, std::move(extra), 0, 0);
}

void SplitBuffers::value_type::replace(std::string_view needle, std::string_view subneedle, std::string&& extra)
{
	auto offset = needle.find(subneedle);
	auto needle_before = offset == needle.npos ? 0 : offset;
	auto needle_after  = offset == needle.npos ? 0 : needle.size() - offset - subneedle.size();

	inject(needle, std::move(extra), needle_before, needle_after);
}

SplitBuffers::const_buffers_type SplitBuffers::value_type::data() const
{
	assert(m_extra.size() + 1 == m_src.size());
	const_buffers_type result{{m_src.front().data(), m_src.front().size()}};

	for (auto i = 0ULL; i < m_extra.size(); i++)
	{
		result.emplace_back(m_extra[i].data(), m_extra[i].size());
		result.emplace_back(m_src[i+1].data(), m_src[i+1].size());
	}

	return result;
}

void SplitBuffers::value_type::inject(
	std::string_view needle,
	std::string&& extra,
	std::size_t needle_before,
	std::size_t needle_after
)
{
	assert(!m_src.empty());
	assert(m_extra.size() + 1 == m_src.size());

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
			m_extra.insert(m_extra.begin() + i, std::move(extra));
			return;
		}
	}

	// special handling for not found
	m_src.emplace_back();
	m_extra.push_back(std::move(extra));
}

std::string SplitBuffers::value_type::str() const
{
	auto buf = data();
	std::string result(boost::asio::buffer_size(buf), '_');
	boost::asio::buffer_copy(boost::asio::buffer(result), buf);
	return result;
}

} // end of namespace
