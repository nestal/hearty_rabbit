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

#include <openssl/evp.h>

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
	open(path, nullptr, path.filename().string(), {}, ec);
}

void BlobObject::open(const boost::filesystem::path& path, const ObjectID* id, std::string_view name, std::string_view mime, std::error_code& ec)
{
	// read the file and calculate the sha and mime_type
	auto blob = MMap::open(path, ec);
	if (!ec)
	{
		m_blob = std::move(blob);
		m_id   = (id ? *id : hash(m_blob.string_view()));
		m_name = name;
		m_mime = (mime.empty() ? deduce_mime(m_blob.string_view()) : mime);
	}
}

ObjectID BlobObject::hash(std::string_view blob)
{
	auto ctx = ::EVP_MD_CTX_new();
	::EVP_DigestInit_ex(ctx, ::EVP_blake2b512(), nullptr);

	std::uint64_t size = blob.size();
	::EVP_DigestUpdate(ctx, &size, sizeof(size));
	::EVP_DigestUpdate(ctx, blob.data(), blob.size());

	unsigned out_size = 0;

	ObjectID result{};
	if constexpr (result.size() == EVP_MAX_MD_SIZE)
	{
		::EVP_DigestFinal_ex(ctx, &result[0], &out_size);
	}
	else
	{
		std::array<unsigned char, EVP_MAX_MD_SIZE> hash;
		::EVP_DigestFinal_ex(ctx, &hash[0], &out_size);
		assert(out_size == result.size());
		::memcpy(&result[0], hash.data(), out_size);
	}

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
		m_id.data(), m_id.size(),
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
		for (auto i = 0ULL ; i+1 < reply.array_size() && !ec ; i+=2)
		{
			if (reply.as_array(i).as_string() == "blob")
			{
				// The blob should be in the next field of the reply array.
				auto blob = reply.as_array(i+1).as_string();

				// Create an anonymous memory mapping to store the blob
				auto new_mem = MMap::allocate(blob.size(), ec);
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

			else
				result.assign_field(reply.as_array(i).as_string(), reply.as_array(i+1).as_string());
		}

		// if redis return OK but we don't have blob, then the object is notvalid
		if (!valid)
			ec = Error::invalid_object;

		// TODO: deduce mime
		if (result.m_mime.empty())
			result.m_mime = deduce_mime(result.blob());

		callback(result, ec);

	}, "HGETALL %b", id.data(), id.size());
}

void BlobObject::load(
	redis::Database& db,
	const ObjectID& id,
	const boost::filesystem::path& path,
	BlobObject::Completion completion
)
{
	db.command([callback=std::move(completion), id, path](redis::Reply reply, std::error_code ec)
	{
		BlobObject result;
		result.m_blob = MMap::open(path, ec);

		for (auto i = 0ULL ; i+1 < reply.array_size() && !ec ; i+=2)
			result.assign_field(reply.as_array(i).as_string(), reply.as_array(i+1).as_string());

		if (result.m_mime.empty())
			result.m_mime = deduce_mime(result.blob());

		callback(result, ec);
	}, "HGETALL %b", id.data(), id.size());
}

void BlobObject::assign(std::string_view blob, std::string_view name, std::error_code& ec)
{
	// Create an anonymous memory mapping to store the blob
	auto new_mem = MMap::allocate(blob.size(), ec);
	if (!ec)
	{
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

void BlobObject::assign_field(std::string_view field, std::string_view value)
{
	// name is optional
	if (field == "name")
		m_name = value;

	// mime is also optional
	else if (field == "mime")
		m_mime = value;
}

std::ostream& operator<<(std::ostream& os, const ObjectID& id)
{
	for (auto ch : id)
	{
		os << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
	}
	return os;
}

} // end of namespace
