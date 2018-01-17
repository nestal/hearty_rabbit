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

#include <boost/filesystem/path.hpp>

#include <openssl/sha.h>

#include <cstddef>
#include <functional>

namespace hrb {

class Database;

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
	~BlobObject();

	BlobObject& operator=(BlobObject&&) = default;
	BlobObject& operator=(const BlobObject&) = delete;

	const ObjectID& ID() const {return m_id;}

	void save(Database& db, std::function<void(BlobObject&)> completion);
	void load(Database& db, const ObjectID& id, std::function<void(BlobObject&)> completion);

	std::string_view blob() const;

private:
	ObjectID    m_id;       //!< SHA1 hash of the blob
	std::string m_name;     //!< Typically the file name of the blob
	std::string m_mime;     //!< Mime-type of the blob, deduced by libmagic

	void *m_mmap{};         //!< Pointer to memory mapped file
	std::size_t m_size{};   //!< File size in bytes.
};

} // end of namespace hrb
