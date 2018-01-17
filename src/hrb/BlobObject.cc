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
	std::error_code ec;
	open(path, ec);
	if (ec)
		throw std::system_error(ec);
}

void BlobObject::open(const boost::filesystem::path &path, std::error_code& ec)
{
	// read the file and calculate the sha and mime_type
	boost::system::error_code bec;
	boost::beast::file_posix file;
	file.open(path.string().c_str(), boost::beast::file_mode::write_existing, bec);
	if (bec)
		ec.assign(bec.value(), bec.category());
	else
	{
		m_blob.open(file.native_handle(), ec);
		if (!ec)
			::SHA1(static_cast<const unsigned char *>(m_blob.data()), m_blob.size(), m_id.data);
	}
}

void BlobObject::save(redis::Database& db, std::function<void(BlobObject&, bool)> completion)
{
	db.command([callback=std::move(completion), this](redis::Reply, std::error_code ec)
	{
		callback(*this, !ec);
	}, "HSET %b blob %b", m_id.data, m_id.size, m_blob.data(), m_blob.size());
}

void BlobObject::load(redis::Database& db, const ObjectID& id, std::function<void(BlobObject&, bool)> completion)
{
	db.command([callback=std::move(completion), id, this](redis::Reply reply, std::error_code ec)
	{
		for (auto i = 0ULL ; i < reply.array_size() && !ec ; i++)
		{
			if (reply.as_array(i).as_string() == "blob")
			{
				// Assign the ID
				m_id = id;

				// The blob should be in the next field of the reply array.
				auto blob = reply.as_array(i+1).as_string();

				// Create an anonymous memory mapping to store the blob
				MMap new_mem;
				new_mem.allocate(blob.size(), ec);
				if (!ec)
					std::memcpy(new_mem.data(), blob.data(), blob.size());

				// everything OK, now commit
				m_blob.swap(new_mem);

				callback(*this, !ec);
				return;
			}
		}

		// TODO: indicate error here
		callback(*this, false);

	}, "HGETALL %b", id.data, id.size);
}

std::string_view BlobObject::blob() const
{
	return {static_cast<const char*>(m_blob.data()), m_blob.size()};
}

} // end of namespace
