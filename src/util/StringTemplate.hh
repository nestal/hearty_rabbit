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

	// wrapper for buffer_copy(data()), useful for unit test
	std::string str() const;

	void set_extra(std::size_t i, std::string&& extra) {m_extra.at(i) = std::move(extra);}

private:
	void inject(std::string_view needle, std::string&& extra, std::size_t needle_before, std::size_t needle_after);

private:
	std::vector<std::string_view>   m_src;
	std::vector<std::string>        m_extra;
};

} // end of namespace hrb
