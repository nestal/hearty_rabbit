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

/// A dynamic HTML response body base on a static file.
/// This class is a HTML response body. It has a writer inner class that returns an array of 3 buffers.
/// These 3 buffers are deduced by searching for a string in the static file. The primary purpose of this
/// class is to inject the <script> tag in HTML files. The constructor will search for the <head> tag and
/// save the offset in the HTML file. Then when the HTML file is sent to the browser, it will be split
/// into three buffers:
///
/// 1. From the beginning to the <head> tag.
/// 2. The additional <script> tag.
/// 3. The rest of the HTML file.
///
/// Only the middle buffer can be assigned dynamically. The first and third buffers are specified by
/// std::string_view
class SplitBuffers
{
public:
	using const_buffers_type = std::array<boost::asio::const_buffer, 3>;

	class value_type
	{
	public:
		explicit value_type(std::string_view file = {}) :
			m_file{file},
			m_offset{m_file.size()}
		{
		}

		value_type(std::string_view file, std::string_view needle, std::string_view xtra) :
			m_file{file}
		{
			extra(needle, std::string{xtra});
		}

		void extra(std::string_view needle, std::string&& extra, std::size_t left = 0, std::size_t right = 0)
		{
			m_left  = left;
			m_right = right;

			m_extra = std::move(extra);
			m_offset = needle.empty() ? m_file.npos : m_file.find(needle);
			if (m_offset != m_file.npos)
				m_offset += needle.size();
			else
				m_offset = m_file.size();
		}

		const_buffers_type data() const
		{
			return {
				boost::asio::const_buffer{m_file.data(),  m_offset - m_left},
				boost::asio::const_buffer{m_extra.data(), m_extra.size()},
				boost::asio::const_buffer{
					m_file.data() + m_offset + m_right,
					m_file.size() - m_offset - m_right
				}
			};
		}

	private:
		std::string_view    m_file;
		std::size_t         m_offset{}, m_left{}, m_right{};
		std::string         m_extra;
	};

	static std::uint64_t size(const value_type& body)
    {
        return buffer_size(body.data());
    }

	class writer
	{
	public:
        using const_buffers_type = SplitBuffers::const_buffers_type;

        template<bool isRequest, class Fields>
        explicit
        writer(boost::beast::http::message<isRequest, SplitBuffers, Fields> const& msg)
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
