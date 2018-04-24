/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

#pragma once

#include <vector>
#include <cstddef>

namespace hrb {

class BinStruct
{
public:
	BinStruct() = default;

	template <typename T>
	void pack(T&& t)
	{
		auto pt = reinterpret_cast<const unsigned char*>(&t);
		m_bytes.insert(m_bytes.end(), pt, pt + sizeof(t));
	}

	template <typename T, typename ... Args>
	void pack(T&& t, Args&& ... args)
	{
		pack(std::forward<T>(t));
		pack(std::forward<Args>(args)...);
	}

	auto data() const {return m_bytes.empty() ? nullptr : &m_bytes[0];}
	auto size() const {return m_bytes.size();}

private:
	std::vector<unsigned char>	m_bytes;
};

} // end of namespace
