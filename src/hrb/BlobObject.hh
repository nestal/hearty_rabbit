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

namespace hrb {
namespace redis {
class Database;
}

struct ObjectID
{
	static const std::size_t size = SHA_DIGEST_LENGTH;
	unsigned char data[size] {};
};

class BlobObject
{
public:
	BlobObject() = default;
	explicit BlobObject(const boost::filesystem::path& path);
	BlobObject(BlobObject&&) = default;
	BlobObject(const BlobObject&) = delete;
	~BlobObject() = default;

	BlobObject& operator=(BlobObject&&) = default;
	BlobObject& operator=(const BlobObject&) = delete;

	const ObjectID& ID() const {return m_id;}
	bool empty() const {return !m_blob.is_opened();}

	void save(redis::Database& db, std::function<void(BlobObject&, bool)> completion);
	void load(redis::Database& db, const ObjectID& id, std::function<void(BlobObject&, bool)> completion);
	void open(const boost::filesystem::path& path, std::error_code& ec);

	std::string_view blob() const;

private:
	ObjectID    m_id;       //!< SHA1 hash of the blob
	std::string m_name;     //!< Typically the file name of the blob
	std::string m_mime;     //!< Mime-type of the blob, deduced by libmagic

	MMap        m_blob;
};

} // end of namespace hrb
