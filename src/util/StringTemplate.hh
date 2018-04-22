/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 4/22/18.
//

#pragma once

#include <boost/asio/buffer.hpp>

#include <string_view>
#include <string>
#include <vector>

namespace hrb {

class StringTemplate
{
public:
	explicit StringTemplate(std::string_view file = {}) :
		m_src(1, file)
	{
	}

	StringTemplate(std::string_view file, std::string_view needle, std::string_view xtra) :
		m_src(1, file)
	{
		replace(needle, std::string{xtra});
	}

	void replace(std::string_view needle, std::string&& extra);
	void replace(std::string_view needle, std::string_view subneedle, std::string&& extra);
	void inject_before(std::string_view needle, std::string&& extra);
	void inject_after(std::string_view needle, std::string&& extra);

	std::vector<boost::asio::const_buffer> data() const;

	using iterator   = std::vector<std::string_view>::const_iterator;
	using value_type = std::string_view;
	iterator begin() const {return m_src.begin();}
	iterator end() const {return m_src.end();}

	template <typename InputIt>
	std::string str(InputIt first, InputIt last) const
	{
		auto it = m_src.begin();
		std::string result{*it++};

		for (;it != m_src.end() && first != last; first++, it++)
		{
			result.append(first->data(), first->size());
			result.append(it->data(), it->size());
		}
		return result;
	}

	std::string str() const {return str(m_extra.begin(), m_extra.end());}

	void set_extra(std::size_t i, std::string&& extra) {m_extra.at(i) = std::move(extra);}

private:
	void inject(std::string_view needle, std::string&& extra, std::size_t needle_before, std::size_t needle_after);

private:
	std::vector<std::string_view>   m_src;
	std::vector<std::string>        m_extra;
};

template <std::size_t N=1>
class InstantiatedStringTemplate
{
public:

private:
	std::array<std::string_view, N+1>   m_src;
	std::array<std::string, N>          m_extra;
};

} // end of namespace hrb
