/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/15/18.
//

#include "BlobObject.hh"

#include "net/RedisDriver.hh"

#include <boost/beast/core/file_posix.hpp>

#include <cassert>
#include <fstream>
#include <iostream>

#include <sys/mman.h>

namespace hrb {

BlobObject::BlobObject(const boost::filesystem::path &path)
{
	// read the file and calculate the sha and mime_type
	boost::system::error_code ec;
	boost::beast::file_posix file;
	file.open(path.string().c_str(), boost::beast::file_mode::read, ec);

	if (ec)
		throw std::system_error(ec);

	struct stat s{};
	fstat(file.native_handle(), &s);
	m_size = static_cast<std::size_t>(s.st_size);

	m_mmap = ::mmap(nullptr, m_size, PROT_READ, MAP_SHARED, file.native_handle(), 0);
	if (m_mmap == MAP_FAILED)
		throw std::system_error(errno, std::system_category());

	::SHA1(static_cast<const unsigned char*>(m_mmap), m_size, m_id.data);
}

BlobObject::~BlobObject()
{
	if (m_mmap)
		::munmap(m_mmap, m_size);
}

void BlobObject::save(RedisDriver& db, std::function<void(BlobObject &)> completion)
{
	db.command([callback=std::move(completion), this](redisReply *)
	{
		callback(*this);
	}, "HSET %b blob %b", m_id.data, m_id.size, m_mmap, m_size);
}

void BlobObject::load(RedisDriver& db, const ObjectID& id, std::function<void(BlobObject&)> completion)
{
	m_id = id;

	db.command([callback=std::move(completion), this](redisReply *reply)
	{
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			for (auto i = 0ULL ; i < reply->elements ; i++)
			{
				if (reply->element[i]->type == REDIS_REPLY_STRING &&
					std::string_view{reply->element[i]->str, static_cast<std::size_t>(reply->element[i]->len)} == "blob")
				{
					std::cout << "get reply " << std::string_view{reply->element[i]->str, static_cast<std::size_t>(reply->element[i]->len)} << std::endl;
					callback(*this);
				}
			}
		}
	}, "HGETALL %b", m_id.data, m_id.size);
}

} // end of namespace
