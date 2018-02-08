/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/8/18.
//

#pragma once

#include "util/MMap.hh"

#include <boost/filesystem/path.hpp>
#include <boost/beast/http/message.hpp>

namespace hrb {

class HTMLTemplate
{
public:
	using value_type = MMap;

	static std::uint64_t size(const value_type& body)
    {
        return body.size();
    }

	class reader
	{
	public:
		template<bool isRequest, class Fields>
        explicit
        reader(boost::beast::http::message<isRequest, HTMLTemplate, Fields>& m)
            : m_body(m.body())
        {
        }

		void init(boost::optional<std::uint64_t>, boost::system::error_code& ec)
		{
			ec.assign(0, ec.category());
		}

		template <typename ConstBufferSeq>
		std::size_t put(const ConstBufferSeq& b, boost::system::error_code& ec)
		{
			// TODO: write to mmap
			ec.clear();
			return buffer_size(b);
		}

		void finish(boost::system::error_code& ec)
		{
			ec.assign(0, ec.category());
		}

	private:
		value_type& m_body;
	};

	class writer
	{
	public:
        using const_buffers_type = boost::asio::const_buffer;

        template<bool isRequest, class Fields>
        explicit
        writer(boost::beast::http::message<isRequest, HTMLTemplate, Fields> const& msg)
            : m_body(msg.body())
        {
        }

        void init(boost::system::error_code& ec)
        {
            ec.assign(0, ec.category());
        }

        boost::optional<std::pair<const_buffers_type, bool>>
        get(boost::system::error_code& ec)
        {
            ec.assign(0, ec.category());

            return {
	            {
		            const_buffers_type{
			            m_body.data(),
			            m_body.size()
		            },
	                false
	            } // pair
            }; // optional
        }

	private:
		const value_type& m_body;
	};
};

} // end of namespace hrb
