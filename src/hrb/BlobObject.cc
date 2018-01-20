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
#include "util/Magic.hh"

#include <boost/beast/core/file_posix.hpp>

#include <cassert>
#include <fstream>
#include <iomanip>

namespace hrb {

BlobObject::BlobObject(const boost::filesystem::path &path)
{
	std::error_code ec;
	open(path, ec);
	if (ec)
		throw std::system_error(ec);
}

BlobObject::BlobObject(std::string_view blob, std::string_view name)
{
	std::error_code ec;
	assign(blob, name, ec);
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
			m_id   = hash(blob.string_view());
			m_blob = std::move(blob);
			m_name = path.filename().string();
			m_mime = deduce_mime(m_blob.string_view());
		}
	}
}

ObjectID BlobObject::hash(std::string_view blob)
{
	ObjectID result{};

	::SHA256_CTX sha{};
	::SHA256_Init(&sha);
	std::uint64_t size = blob.size();
	::SHA256_Update(&sha, &size, sizeof(size));
	::SHA256_Update(&sha, blob.data(), blob.size());
	::SHA256_Final(result.data, &sha);

	return result;
}

void BlobObject::save(redis::Database& db, Completion completion)
{
	db.command(
		[callback=std::move(completion), this](redis::Reply, std::error_code ec)
		{
			callback(*this, ec);
		},
		"HSET %b blob %b name %b mime %b",
		m_id.data, m_id.size,
		m_blob.data(), m_blob.size(),
		m_name.c_str(), m_name.size(),
		m_mime.c_str(), m_mime.size()
	);
}

void BlobObject::load(redis::Database& db, const ObjectID& id, Completion completion)
{
	db.command([callback=std::move(completion), id](redis::Reply reply, std::error_code ec)
	{
		BlobObject result;

		// Keep the redis error code if it is non-zero
		if (!ec && reply.array_size() == 0)
			ec = Error::object_not_exist;

		bool valid = false;
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

					// blob is require
					valid = true;
				}
			}

			// name is optional
			else if (reply.as_array(i).as_string() == "name")
				result.m_name = reply.as_array(i+1).as_string();

			// mime is also optional
			else if (reply.as_array(i).as_string() == "mime")
				result.m_mime = reply.as_array(i+1).as_string();
		}

		// if redis return OK but we don't have blob, then the object is notvalid
		if (!valid)
			ec = Error::invalid_object;

		// TODO: deduce mime

		callback(result, ec);

	}, "HGETALL %b", id.data, id.size);
}

void BlobObject::assign(std::string_view blob, std::string_view name, std::error_code& ec)
{
	// Create an anonymous memory mapping to store the blob
	MMap new_mem;
	new_mem.allocate(blob.size(), ec);
	if (!ec)
	{
		// everything OK, now commit
		std::memcpy(new_mem.data(), blob.data(), blob.size());
		m_blob = std::move(new_mem);
		m_id   = hash(m_blob.string_view());
		m_name = name;
		m_mime = deduce_mime(m_blob.string_view());
	}
}

std::string BlobObject::deduce_mime(std::string_view blob)
{
	static const thread_local Magic magic;
	return std::string{magic.mime(blob)};
}

std::string_view BlobObject::blob() const
{
	return {static_cast<const char*>(m_blob.data()), m_blob.size()};
}

std::ostream& operator<<(std::ostream& os, const ObjectID& id)
{
	for (auto ch : id.data)
	{
		os << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
	}
	return os;
}

} // end of namespace
