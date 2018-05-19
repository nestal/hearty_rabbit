/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/20/18.
//

#include "Magic.hh"

namespace hrb {

Magic::Magic() : m_cookie{::magic_open(MAGIC_MIME_TYPE)}
{
	::magic_load(m_cookie, nullptr);
}

Magic::~Magic()
{
	::magic_close(m_cookie);
}

std::string_view Magic::mime(boost::asio::const_buffer buf) const
{
	return mime(buf.data(), buf.size());
}

std::string_view Magic::mime(const void *buffer, std::size_t size) const
{
	return ::magic_buffer(m_cookie, buffer, size);
}

std::string_view Magic::mime(boost::beast::file_posix::native_handle_type fd) const
{
	return ::magic_descriptor(m_cookie, fd);
}

std::string_view Magic::mime(const boost::filesystem::path& path) const
{
	return ::magic_file(m_cookie, path.string().c_str());
}

} // end of namespace
