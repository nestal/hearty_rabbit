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

class FileBuffers
{
public:
	using const_buffers_type = std::array<boost::asio::const_buffer, 3>;

	class value_type
	{
	public:
		explicit value_type(std::string_view file = {}, std::string_view needle = {}, std::string_view extra = {}) :
			m_file{file},
			m_extra{extra}
		{
			m_offset = needle.size() > 0 ? m_file.find(needle) : m_file.npos;
			if (m_offset != m_file.npos)
				m_offset += needle.size();
			else
				m_offset = m_file.size();
		}

		const_buffers_type data() const
		{
			return {
				boost::asio::const_buffer{m_file.data(), m_offset},
				boost::asio::const_buffer{m_extra.data(), m_extra.size()},
				boost::asio::const_buffer{m_file.data() + m_offset, m_file.size() - m_offset}
			};
		}

	private:
		std::string_view    m_file;
		std::size_t         m_offset{};
		std::string         m_extra;
	};

	static std::uint64_t size(const value_type& body)
    {
        return buffer_size(body.data());
    }

	class reader
	{
	public:
		template<bool isRequest, class Fields>
        explicit
        reader(boost::beast::http::message<isRequest, FileBuffers, Fields>& m)
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
        using const_buffers_type = FileBuffers::const_buffers_type;

        template<bool isRequest, class Fields>
        explicit
        writer(boost::beast::http::message<isRequest, FileBuffers, Fields> const& msg)
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
	            {m_body.data(), false} // pair
            }; // optional
        }

	private:
		const value_type& m_body;
	};
};

} // end of namespace hrb
