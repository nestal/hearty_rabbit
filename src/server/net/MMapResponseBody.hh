/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/24/18.
//

#pragma once

#include "common/util/MMap.hh"

#include <boost/beast/http/message.hpp>

namespace hrb {

class MMapResponseBody
{
public:
	using value_type = MMap;

	static std::uint64_t size(const value_type& body);

	class writer
	{
	public:
        using const_buffers_type = boost::asio::const_buffer;

        template<bool isRequest, class Fields>
        explicit
        writer(boost::beast::http::header<isRequest, Fields> const&, value_type& body)
            : m_body(body)
        {
        }

        void init(boost::system::error_code& ec);

        boost::optional<std::pair<const_buffers_type, bool>>
        get(boost::system::error_code& ec);

	private:
		const value_type& m_body;
	};
};

} // end of namespace hrb
