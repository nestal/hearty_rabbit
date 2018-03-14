/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/17/18.
//

#pragma once

#include "ObjectID.hh"
#include "util/FS.hh"
#include "util/Magic.hh"
#include "util/Size2D.hh"

#include <boost/beast/http/message.hpp>
#include <rapidjson/document.h>

#include <optional>

namespace hrb {

class ObjectID;
class BlobFile;
class UploadFile;
class MMapResponseBody;
class Magic;
class CollEntry;

class BlobDatabase
{
public:
	using BlobResponse = boost::beast::http::response<MMapResponseBody>;

public:
	explicit BlobDatabase(const fs::path& base, const Size2D& resize_img);

	void prepare_upload(UploadFile& result, std::error_code& ec) const;
	BlobFile save(UploadFile&& tmp, std::string_view filename, std::error_code& ec);

	fs::path dest(const ObjectID& id, std::string_view rendition = {}) const;
	std::string load_meta_json(const ObjectID& id) const;

	BlobResponse response(
		ObjectID id,
		unsigned version,
		std::string_view mine,
		std::string_view etag,
		std::string_view rendition = {}
	) const;

private:
	static void set_cache_control(BlobResponse& res, const ObjectID& id);

private:
	fs::path    m_base;
	Magic       m_magic;
	Size2D        m_resize_img;
};

} // end of namespace hrb
