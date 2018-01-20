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
	::magic_compile(m_cookie, nullptr);
}

Magic::~Magic()
{
	::magic_close(m_cookie);
}

std::string_view Magic::mime(std::string_view buf) const
{
	return mime(buf.data(), buf.size());
}

std::string_view Magic::mime(const void *buffer, std::size_t size) const
{
	return ::magic_buffer(m_cookie, buffer, size);
}

} // end of namespace
