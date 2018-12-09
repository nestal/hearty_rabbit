/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/9/18.
//

#pragma once

#include "net/Request.hh"
#include "common/FS.hh"

#include <boost/beast/core/flat_buffer.hpp>

#include <cstring>
#include <fstream>

namespace hrb {

template <typename Body, typename Allocator>
auto flatten_content(http::response<Body, http::basic_fields<Allocator>>&& res)
{
	boost::system::error_code ec;
	boost::beast::flat_buffer fbuf;

	// spend a lot of time to get this line to compile...
	// and even more for boost 1.67
	typename http::response<Body, http::basic_fields<Allocator>>::body_type::writer writer{res, res.body()};
	while (auto buf = writer.get(ec))
	{
		if (!ec)
		{
			auto size = buffer_size(buf->first);
			buffer_copy(fbuf.prepare(size), buf->first);
			fbuf.commit(size);
		}
		if (ec || !buf->second)
			break;
	}
	return fbuf;
}

template <typename ConstBuffer>
bool check_file_content(const fs::path& file, ConstBuffer content)
{
	// open index.html and compare
	std::ifstream index{file.string()};
	char buf[1024];
	while (auto count = index.rdbuf()->sgetn(buf, sizeof(buf)))
	{
		auto result = std::memcmp(buf, content.data(), static_cast<std::size_t>(count));
		if (result != 0)
			return false;

		content += count;
	}

	return content.size() == 0;
}

template <typename Body, typename Allocator>
bool check_resource_content(
	const fs::path& file,
	http::response<Body, http::basic_fields<Allocator>>&& res
)
{
	auto content = flatten_content(std::move(res));
	return check_file_content(file, content.data());
}

} // end of namespace
