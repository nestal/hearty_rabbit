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

#include <cstddef>
#include <functional>

namespace hrb {

class RedisDriver;

struct ObjectID
{
	static const std::size_t size = 20;
	unsigned char data[size] {};
};

class BlobObject
{
public:
	BlobObject() = default;
	explicit BlobObject(const boost::filesystem::path& path);

	const ObjectID& ID() const {return m_id;}

	void Save(RedisDriver& db, std::function<void(BlobObject&)> completion);
	void Load(RedisDriver& db, std::function<void(BlobObject&)> completion);

private:
	ObjectID    m_id;       //!< SHA1 hash of the blob
	std::string m_name;     //!< Typically the file name of the blob
	std::string m_mime;     //!< Mime-type of the blob, deduced by libmagic
};

} // end of namespace hrb
