/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/20/18.
//

#pragma once

#include "common/FS.hh"
#include "common/BufferView.hh"

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/file.hpp>

#include <magic.h>
#include <string_view>

namespace hrb {

class Magic
{
public:
	Magic();
	Magic(const Magic&) = delete;
	Magic(Magic&&) = delete;
	~Magic();

	std::string_view mime(const void *buffer, std::size_t size) const;
	std::string_view mime(boost::asio::const_buffer buf) const;
	std::string_view mime(BufferView buf) const;
	std::string_view mime(boost::beast::file::native_handle_type fd) const;
	std::string_view mime(const fs::path& path) const;

	static const Magic& instance();

private:
	::magic_t m_cookie;
};

} // end of namespace
