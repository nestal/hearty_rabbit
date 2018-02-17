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

#include "crypto/Blake2.hh"
#include "util/Error.hh"
#include "util/Magic.hh"

#include <boost/algorithm/hex.hpp>

#include <openssl/evp.h>

#include <cassert>
#include <fstream>
#include <iomanip>

namespace hrb {

namespace {

static constexpr std::array<unsigned char, 5> key_prefix = {'b','l','o','b', ':'};
using ObjectRedisKey = std::array<unsigned char, Blake2::size + key_prefix.size()>;

ObjectRedisKey redis_key(const ObjectID& id)
{
	ObjectRedisKey key = {};
	std::copy(key_prefix.begin(), key_prefix.end(), key.begin() );
	std::copy(id.begin(), id.end(), key.begin() + key_prefix.size());
	return key;
}

} // end of local namespace

BlobObject::BlobObject(const boost::filesystem::path &path)
{
	std::error_code ec;
	open(path, ec);
	if (ec)
		throw std::system_error(ec);
}

BlobObject::BlobObject(boost::asio::const_buffer blob, std::string_view name)
{
	std::error_code ec;
	assign(blob, name, ec);
	if (ec)
		throw std::system_error(ec);
}

BlobObject::BlobObject(std::string&& blob, std::string_view name)
{
	m_id   = hash({blob.data(), blob.size()});
	m_name = name;
	m_mime = deduce_mime({blob.data(), blob.size()});
	m_blob = std::move(blob);

}

void BlobObject::open(const boost::filesystem::path &path, std::error_code& ec)
{
	open(path, nullptr, path.filename().string(), {}, ec);
}

void BlobObject::open(const boost::filesystem::path& path, const ObjectID* id, std::string_view name, std::string_view mime, std::error_code& ec)
{
	// read the file and calculate the sha and mime_type
	auto data = MMap::open(path, ec);
	if (!ec)
	{
		m_id   = (id ? *id : hash(data.blob()));
		m_name = name;
		m_mime = (mime.empty() ? deduce_mime(data.blob()) : mime);
		m_blob = std::move(data);
	}
}

ObjectID BlobObject::hash(boost::asio::const_buffer blob)
{
	Blake2 sha3;

	std::uint64_t size = blob.size();
	sha3.update(&size, sizeof(size));
	sha3.update(blob.data(), blob.size());

	return ObjectID{sha3.finalize()};
}

void BlobObject::erase(redis::Connection& db, BlobObject::Completion completion)
{
	db.command(
		[callback=std::move(completion), this](redis::Reply, std::error_code&& ec)
		{
			callback(*this, std::move(ec));
		},
		"DEL %b%b",
		key_prefix.data(), key_prefix.size(),
		m_id.data(), m_id.size()
	);
}

void BlobObject::save(redis::Connection& db, Completion completion)
{
	auto data = blob();
	db.command(
		[callback=std::move(completion), this](redis::Reply, std::error_code&& ec)
		{
			callback(*this, std::move(ec));
		},
		"HSET %b%b blob %b name %b mime %b",
		key_prefix.data(), key_prefix.size(),
		m_id.data(), m_id.size(),
		data.data(), data.size(),
		m_name.c_str(), m_name.size(),
		m_mime.c_str(), m_mime.size()
	);
}

void BlobObject::load(redis::Connection& db, const ObjectID& id, Completion completion)
{
	db.command(
		[callback=std::move(completion), id](redis::Reply reply, std::error_code ec)
		{
			BlobObject result;

			// Keep the redis error code if it is non-zero
			if (!ec && reply.array_size() == 0)
				ec = Error::object_not_exist;

			auto [blob_reply] = reply.map_kv_pair("blob");
			if (auto blob = blob_reply.as_string(); !blob.empty())
			{
				result.m_id   = id;
				result.m_blob = blob_reply;

				reply.foreach_kv_pair([&result](auto&& field, auto&& value)
				{
					if (field != "blob")
						result.assign_field(field, value.as_string());
				});

				// deduce mime if it is not present in database
				if (result.m_mime.empty())
					result.m_mime = deduce_mime(result.blob());
			}

			// if redis return OK but we don't have blob, then the object is not valid
			else
				ec = Error::invalid_object;

			callback(result, ec);
		},
		"HGETALL %b%b",
		key_prefix.data(), key_prefix.size(),
		id.data(), id.size()
	);
}

void BlobObject::load(
	redis::Connection& db,
	const ObjectID& id,
	const boost::filesystem::path& path,
	BlobObject::Completion completion
)
{
	db.command(
		[callback=std::move(completion), id, path](redis::Reply reply, std::error_code ec)
		{
			BlobObject result;
			result.m_blob = MMap::open(path, ec);

			for (auto i = 0ULL ; i+1 < reply.array_size() && !ec ; i+=2)
				result.assign_field(reply.as_array(i).as_string(), reply.as_array(i+1).as_string());

			if (result.m_mime.empty())
				result.m_mime = deduce_mime(result.blob());

			callback(result, ec);
		}, "HGETALL %b%b",
		key_prefix.data(), key_prefix.size(),
		id.data(), id.size()
	);
}

void BlobObject::assign(boost::asio::const_buffer data, std::string_view name, std::error_code& ec)
{
	m_id   = hash(data);
	m_name = name;
	m_mime = deduce_mime(data);
	m_blob = Vec(
		static_cast<const char*>(data.data()),
		static_cast<const char*>(data.data()) + data.size()
	);
}

std::string BlobObject::deduce_mime(boost::asio::const_buffer blob)
{
	static const thread_local Magic magic;
	return std::string{magic.mime(blob)};
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

bool BlobObject::empty() const
{
	struct IsEmpty
	{
		bool operator()(const MMap& mmap) const noexcept {return !mmap.is_opened();}
		bool operator()(const Vec& vec) const noexcept {return vec.empty();}
		bool operator()(const std::string& s) const noexcept {return s.empty();}
		bool operator()(const redis::Reply& reply) const noexcept
		{
			return reply.as_string().size() == 0;
		}
	};
	return std::visit(IsEmpty(), m_blob);
}

boost::asio::const_buffer BlobObject::blob() const
{
	struct GetBuffer
	{
		boost::asio::const_buffer operator()(const MMap& mmap) const noexcept
		{
			return mmap.blob();
		}
		boost::asio::const_buffer operator()(const Vec& v) const noexcept
		{
			return {&v[0], v.size()};
		}
		boost::asio::const_buffer operator()(const std::string& s) const noexcept
		{
			return {&s[0], s.size()};
		}
		boost::asio::const_buffer operator()(const redis::Reply& reply) const noexcept
		{
			auto s = reply.as_string();
			return {s.data(), s.size()};
		}
	};
	return std::visit(GetBuffer(), m_blob);
}

std::string_view BlobObject::string() const
{
	auto data = blob();
	return {static_cast<const char*>(data.data()), data.size()};
}

std::string to_hex(const ObjectID& id)
{
	std::string result(id.size()*2, '\0');
	boost::algorithm::hex_lower(id.begin(), id.end(), result.begin());
	return result;
}

ObjectID hex_to_object_id(std::string_view hex)
{
	ObjectID result{};
	if (hex.size() == result.size()*2)
		boost::algorithm::unhex(hex.begin(), hex.end(), result.begin());
	return result;
}

bool operator==(const ObjectID& id1, const ObjectID& id2)
{
	static_assert(id1.size() == id2.size());    // isn't it obvious?
	return std::memcmp(id1.data(), id2.data(), id1.size()) == 0;
}

bool operator!=(const ObjectID& id1, const ObjectID& id2)
{
	static_assert(id1.size() == id2.size());    // isn't it obvious?
	return std::memcmp(id1.data(), id2.data(), id1.size()) != 0;
}

std::ostream& operator<<(std::ostream& os, const ObjectID& id)
{
	return os << to_hex(id);
}

} // end of namespace
