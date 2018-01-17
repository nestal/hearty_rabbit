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

#include "net/Redis.hh"

#include <boost/beast/core/file_posix.hpp>

#include <cassert>
#include <fstream>

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

void BlobObject::save(redis::Database& db, std::function<void(BlobObject&, bool)> completion)
{
	db.command([callback=std::move(completion), this](redis::Reply, std::error_code ec)
	{
		callback(*this, !ec);
	}, "HSET %b blob %b", m_id.data, m_id.size, m_mmap, m_size);
}

void BlobObject::load(redis::Database& db, const ObjectID& id, std::function<void(BlobObject&, bool)> completion)
{
	db.command([callback=std::move(completion), id, this](redis::Reply reply, std::error_code)
	{
		for (auto i = 0ULL ; i < reply.array_size() ; i++)
		{
			if (reply.as_array(i).as_string() == "blob")
			{
				// Assign the ID
				m_id = id;

				// The blob should be in the next field of the reply array.
				auto blob = reply.as_array(i+1).as_string();

				// Create an anonymous memory mapping to store the blob
				m_mmap = ::mmap(nullptr, blob.size(), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
				if (m_mmap == MAP_FAILED)
					throw std::system_error(errno, std::system_category());

				// Copy the blob to the anonymous memory mapping
				std::memcpy(m_mmap, blob.data(), blob.size());
				m_size = blob.size();

				callback(*this, true);
				return;
			}
		}

		// TODO: indicate error here
		callback(*this, false);

	}, "HGETALL %b", id.data, id.size);
}

std::string_view BlobObject::blob() const
{
	return {static_cast<const char*>(m_mmap), m_size};
}

} // end of namespace
