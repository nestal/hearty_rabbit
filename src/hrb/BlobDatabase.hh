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

public:
	explicit BlobDatabase(const fs::path& base);

	void prepare_upload(UploadFile& result, std::error_code& ec) const;
	ObjectID save(const UploadFile& tmp, std::error_code& ec);

	fs::path dest(ObjectID id, std::string_view rendition = {}) const;

	BlobResponse response(
		ObjectID id,
		unsigned version,
		std::string_view etag,
		std::string_view rendition = {}
	) const;

private:
	static void set_cache_control(BlobResponse& res, const ObjectID& id);
	void save_meta(const ObjectID& id, const UploadFile& tmp);

private:
	fs::path    m_base;
	Magic       m_magic;
};

} // end of namespace hrb
