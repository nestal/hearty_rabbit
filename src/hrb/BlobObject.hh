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

#include "util/MMap.hh"

#include <boost/filesystem/path.hpp>
#include <openssl/sha.h>

#include <cstddef>
#include <functional>
#include <system_error>

namespace hrb {
namespace redis {
class Database;
}

struct ObjectID
{
	static const std::size_t size = SHA256_DIGEST_LENGTH;
	unsigned char data[size] {};
};

class BlobObject
{
public:
	BlobObject() = default;
	explicit BlobObject(const boost::filesystem::path& path);
	BlobObject(std::string_view blob, std::string_view name);
	BlobObject(BlobObject&&) = default;
	BlobObject(const BlobObject&) = delete;
	~BlobObject() = default;

	BlobObject& operator=(BlobObject&&) = default;
	BlobObject& operator=(const BlobObject&) = delete;

	const ObjectID& ID() const {return m_id;}
	bool empty() const {return !m_blob.is_opened();}

	using Completion = std::function<void(BlobObject&, std::error_code ec)>;

	static void load(redis::Database& db, const ObjectID& id, Completion completion);
	void save(redis::Database& db, Completion completion);
	void open(const boost::filesystem::path& path, std::error_code& ec);
	void assign(std::string_view blob, std::string_view name, std::error_code& ec);

	std::string_view blob() const;
	const std::string& name() const {return m_name;}

private:
	static ObjectID hash(std::string_view blob);

private:
	ObjectID    m_id;       //!< SHA1 hash of the blob
	std::string m_name;     //!< Typically the file name of the blob
	std::string m_mime;     //!< Mime-type of the blob, deduced by libmagic

	MMap        m_blob;
};

std::ostream& operator<<(std::ostream& os, const ObjectID& id);

} // end of namespace hrb
