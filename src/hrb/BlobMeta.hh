/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/26/18.
//

#pragma once

#include <string>
#include <rapidjson/document.h>
#include <boost/asio/buffer.hpp>

namespace hrb {

class Magic;

class BlobMeta
{
public:
	BlobMeta() = default;
	BlobMeta(const BlobMeta&) = default;
	BlobMeta(BlobMeta&&) = default;
	~BlobMeta() = default;

	BlobMeta& operator=(const BlobMeta&) = default;
	BlobMeta& operator=(BlobMeta&&) = default;

	static BlobMeta deduce_meta(boost::asio::const_buffer blob, const Magic& magic);
	static BlobMeta load(rapidjson::Document& json);

	rapidjson::Document serialize() const;

	const std::string& mime() const {return m_mime;}
//	long orientation() const {return m_orientation;}
	const std::string& filename() const {return m_filename;}

	void filename(std::string_view fn) {m_filename = fn;}

private:
	std::string m_mime{"application/octet-stream"};
//	long m_orientation{1};
	std::string m_filename;
};

} // end of namespace hrb
