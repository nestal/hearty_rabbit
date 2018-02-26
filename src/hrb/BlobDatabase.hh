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

#include <boost/beast/http/message.hpp>
#include <rapidjson/document.h>

#include <optional>

namespace hrb {

class ObjectID;
class BlobObject;
class UploadFile;
class MMapResponseBody;
class Magic;

class BlobDatabase
{
public:
	using BlobResponse = boost::beast::http::response<MMapResponseBody>;

	struct Meta
	{
		std::string mime{"application/octet-stream"};
		int orientation{1};
	};

public:
	explicit BlobDatabase(const fs::path& base);

	void prepare_upload(UploadFile& result, std::error_code& ec) const;
	ObjectID save(const UploadFile& tmp, std::error_code& ec);

	fs::path dest(const ObjectID& id, std::string_view rendition = {}) const;
	std::optional<Meta> load_meta(const ObjectID& id) const;
	std::optional<std::string> load_meta_json(const ObjectID& id) const;

	BlobResponse response(
		ObjectID id,
		unsigned version,
		std::string_view etag,
		std::string_view rendition = {}
	) const;

private:
	static void set_cache_control(BlobResponse& res, const ObjectID& id);
	std::optional<Meta> deduce_meta(const UploadFile& tmp) const;
	std::optional<Meta> deduce_meta(boost::asio::const_buffer blob) const;
	std::optional<Meta> save_meta(const fs::path& dest_path, const UploadFile& tmp) const;
	std::optional<Meta> load_meta(const fs::path& dest_path) const;

private:
	fs::path    m_base;
	Magic       m_magic;
};

} // end of namespace hrb
