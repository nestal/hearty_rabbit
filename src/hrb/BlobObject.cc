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
#include "util/Error.hh"

#include <boost/beast/core/file_posix.hpp>

#include <cassert>
#include <fstream>

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
	file.open(path.string().c_str(), boost::beast::file_mode::read, bec);
	if (bec)
		ec.assign(bec.value(), bec.category());
	else
	{
		MMap blob;
		blob.open(file.native_handle(), ec);
		if (!ec)
		{
			::SHA256_CTX sha{};
			::SHA256_Init(&sha);
			auto size = blob.size();
			::SHA256_Update(&sha, &size, sizeof(size));
			::SHA256_Update(&sha, blob.data(), blob.size());
			::SHA256_Final(m_id.data, &sha);

			m_blob = std::move(blob);
		}
	}
}

void BlobObject::save(redis::Database& db, Completion completion)
{
	db.command([callback=std::move(completion), this](redis::Reply, std::error_code ec)
	{
		callback(*this, ec);
	}, "HSET %b blob %b", m_id.data, m_id.size, m_blob.data(), m_blob.size());
}

void BlobObject::load(redis::Database& db, const ObjectID& id, Completion completion)
{
	db.command([callback=std::move(completion), id](redis::Reply reply, std::error_code ec)
	{
		BlobObject result;

		// Keep the redis error code if it is non-zero
		if (!ec && reply.array_size() == 0)
			ec = Error::object_not_exist;

		for (auto i = 0ULL ; i < reply.array_size() && !ec ; i++)
		{
			if (reply.as_array(i).as_string() == "blob")
			{
				// The blob should be in the next field of the reply array.
				auto blob = reply.as_array(i+1).as_string();

				// Create an anonymous memory mapping to store the blob
				MMap new_mem;
				new_mem.allocate(blob.size(), ec);
				if (!ec)
				{
					// everything OK, now commit
					std::memcpy(new_mem.data(), blob.data(), blob.size());
					result.m_id = id;
					result.m_blob = std::move(new_mem);
				}
				break;
			}
		}

		callback(result, ec);

	}, "HGETALL %b", id.data, id.size);
}

std::string_view BlobObject::blob() const
{
	return {static_cast<const char*>(m_blob.data()), m_blob.size()};
}

} // end of namespace
