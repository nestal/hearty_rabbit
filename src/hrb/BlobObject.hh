/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/15/18.
//

#pragma once

#include "crypto/Blake2.hh"
#include "util/MMap.hh"
#include "net/Redis.hh"

#include <boost/filesystem/path.hpp>
#include <boost/asio/buffer.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <system_error>
#include <variant>
#include <vector>

namespace hrb {
namespace redis {
class Connection;
}

// If use a typedef (or using), then the argument-dependent lookup (ADL) will not
// work for operator<<
struct ObjectID : std::array<unsigned char, Blake2::size>
{
	using array::array;
	explicit ObjectID(const array& ar) : array{ar} {}
};

static_assert(std::is_standard_layout<ObjectID>::value);

std::string to_hex(const ObjectID& id);
ObjectID hex_to_object_id(std::string_view base64);

bool operator==(const ObjectID& id1, const ObjectID& id2);
bool operator!=(const ObjectID& id1, const ObjectID& id2);

class BlobObject
{
public:
	BlobObject() = default;
	explicit BlobObject(const boost::filesystem::path& path);
	BlobObject(boost::asio::const_buffer blob, std::string_view name);
	BlobObject(std::string&& blob, std::string_view name);
	BlobObject(BlobObject&&) = default;
	BlobObject(const BlobObject&) = delete;
	~BlobObject() = default;

	BlobObject& operator=(BlobObject&&) = default;
	BlobObject& operator=(const BlobObject&) = delete;

	const ObjectID& ID() const {return m_id;}
	bool empty() const;

	using Completion = std::function<void(BlobObject&, std::error_code ec)>;

	// Load BlobObject from redis database
	static void load(redis::Connection& db, const ObjectID& id, Completion completion);
	static void load(
		redis::Connection& db,
		const ObjectID& id,
		const boost::filesystem::path& path,
		Completion completion
	);

	void save(redis::Connection& db, Completion completion);
	void erase(redis::Connection& db, Completion completion);
	void open(const boost::filesystem::path& path, std::error_code& ec);
	void assign(boost::asio::const_buffer blob, std::string_view name, std::error_code& ec);

	boost::asio::const_buffer blob() const;
	std::string_view string() const;
	const std::string& name() const {return m_name;}
	const std::string& mime() const {return m_mime;}

private:
	static ObjectID hash(boost::asio::const_buffer blob);
	static std::string deduce_mime(boost::asio::const_buffer blob);
	void open(
		const boost::filesystem::path& path,
		const ObjectID* id,
		std::string_view name,
		std::string_view mime,
		std::error_code& ec
	);
	void assign_field(std::string_view field, std::string_view value);

private:
	ObjectID    m_id{};     //!< SHA512 hash of the blob
	std::string m_name;     //!< Typically the file name of the blob
	std::string m_mime;     //!< Mime-type of the blob, deduced by libmagic

	using Vec = std::vector<unsigned char>;
	std::variant<MMap, Vec, std::string, redis::Reply> m_blob;
};

std::ostream& operator<<(std::ostream& os, const ObjectID& id);

} // end of namespace hrb
