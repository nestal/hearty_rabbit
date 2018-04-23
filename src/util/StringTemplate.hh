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

	StringTemplate(std::string_view file, std::string_view needle) :
		m_src(1, file)
	{
		replace(needle);
	}

	// feel free to copy
	StringTemplate(StringTemplate&&) = default;
	StringTemplate(const StringTemplate&) = default;
	StringTemplate& operator=(StringTemplate&&) = default;
	StringTemplate& operator=(const StringTemplate&) = default;
	~StringTemplate() = default;

	void replace(std::string_view needle);
	void replace(std::string_view needle, std::string_view subneedle);
	void inject_before(std::string_view needle);
	void inject_after(std::string_view needle);

	using iterator   = std::vector<std::string_view>::const_iterator;
	using value_type = std::string_view;
	iterator begin() const {return m_src.begin();}
	iterator end() const {return m_src.end();}

private:
	void inject(std::string_view needle, std::size_t needle_before, std::size_t needle_after);

private:
	std::vector<std::string_view>   m_src;
};

template <std::size_t N=1>
class InstantiatedStringTemplate
{
public:
	using const_buffers_type = std::vector<boost::asio::const_buffer>;

	static constexpr auto segment_count() {return N+1;}

public:
	InstantiatedStringTemplate() = default;

	template <typename FwdIt>
	InstantiatedStringTemplate(FwdIt first, FwdIt last)
	{
		if (std::distance(first, last) > m_src.size())
			throw std::out_of_range("size mismatch");

		std::copy(first, last, m_src.begin());
	}

	void set_extra(std::size_t i, std::string&& extra) {m_extra.at(i) = std::move(extra);}

	auto data() const
	{
		auto src   = m_src.begin();
		auto extra = m_extra.begin();

		const_buffers_type result{boost::asio::buffer(src->data(), src->size())};
		src++;

		for (;src != m_src.end() && extra != m_extra.end(); src++, extra++)
		{
			if (extra->size() > 0)
				result.emplace_back(extra->data(), extra->size());
			if (src->size() > 0)
				result.emplace_back(src->data(), src->size());
		}
		return result;
	}

	std::string str() const
	{
		auto bufs = data();
		std::string result(boost::asio::buffer_size(bufs), ' ');
		boost::asio::buffer_copy(boost::asio::buffer(result), bufs);
		return result;
	}

private:
	std::array<std::string_view, segment_count()>   m_src;
	std::array<std::string, N>                      m_extra;
};

} // end of namespace hrb
