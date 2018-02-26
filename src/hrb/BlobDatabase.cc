/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/17/18.
//

#include "BlobDatabase.hh"
#include "UploadFile.hh"

#include "net/MMapResponseBody.hh"

#include "util/Log.hh"
#include "util/Magic.hh"
#include "util/Exif.hh"

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/pointer.h>

#include <sstream>
#include <fstream>

namespace hrb {
namespace {
const std::string_view default_rendition = "master";
const std::string_view metafile = "meta";
}

BlobDatabase::BlobDatabase(const fs::path& base) : m_base{base}
{
	if (exists(base) && !is_directory(base))
		throw std::system_error(std::make_error_code(std::errc::file_exists));

	if (!exists(base))
		create_directories(base);
}

void BlobDatabase::prepare_upload(UploadFile& result, std::error_code& ec) const
{
	boost::system::error_code err;

	result.open(m_base.string().c_str(), err);
	if (err)
		ec.assign(err.value(), err.category());
}

ObjectID BlobDatabase::save(const UploadFile& tmp, std::error_code& ec)
{
	auto id = tmp.ID();
	auto dest_path = dest(id);

	boost::system::error_code bec;
	if (!exists(dest_path.parent_path()))
		create_directories(dest_path.parent_path(), bec);
	if (bec)
		Log(LOG_WARNING, "create directory %1% %2%", bec, bec.message());

	ec.assign(bec.value(), bec.category());

	save_meta(dest_path, deduce_meta(tmp));

	if (!ec)
	{
		tmp.linkat(dest_path, ec);
		if (ec == std::errc::file_exists)
		{
			// TODO: check file size before accepting
			Log(LOG_INFO, "linkat() %1% exists", dest_path);
			ec.clear();
		}
		else if (ec)
			Log(LOG_WARNING, "linkat() %1% %2%", ec, ec.message());
	}

	return id;
}

fs::path BlobDatabase::dest(const ObjectID& id, std::string_view) const
{
	auto hex = to_hex(id);
	assert(hex.size() > 2);

	return m_base / hex.substr(0, 2) / hex / std::string{default_rendition};
}

BlobDatabase::BlobResponse BlobDatabase::response(
	ObjectID id,
	unsigned version,
	std::string_view etag,
	std::string_view rendition
) const
{
	if (etag == to_quoted_hex(id))
	{
		http::response<MMapResponseBody> res{http::status::not_modified, version};
		set_cache_control(res, id);
		return res;
	}

	auto path = dest(id);

	std::error_code ec;
	auto mmap = MMap::open(path, ec);
	if (ec)
		return BlobResponse{http::status::not_found, version};

	auto meta = load_meta(id);
	if (!meta)
	{
		meta = deduce_meta(mmap.blob());
		if (!meta)
		{
			Log(LOG_WARNING, "cannot load metadata from file %1%", path);
			meta.emplace(Meta{});
		}

		save_meta(path.parent_path()/std::string{metafile}, *meta);
	}

	// Advice the kernel that we only read the memory in one pass
	mmap.cache();

	BlobResponse res{
		std::piecewise_construct,
		std::make_tuple(std::move(mmap)),
		std::make_tuple(http::status::ok, version)
	};
	res.set(http::field::content_type, meta->mime);
	set_cache_control(res, id);
	return res;

}

void BlobDatabase::set_cache_control(BlobResponse& res, const ObjectID& id)
{
	res.set(http::field::cache_control, "private, max-age=31536000, immutable");
	res.set(http::field::etag, to_quoted_hex(id));
}

BlobDatabase::Meta BlobDatabase::deduce_meta(const UploadFile& tmp) const
{
	// Deduce mime type and orientation (if it is a JPEG)
	std::error_code ec;
	auto mmap = MMap::open(tmp.native_handle(), ec);
	if (ec)
	{
		Log(LOG_WARNING, "cannot open tmp file to deduce meta");
		return {};
	}

	return deduce_meta(mmap.blob());
}

BlobDatabase::Meta BlobDatabase::deduce_meta(boost::asio::const_buffer blob) const
{
	Meta meta;
	meta.mime = m_magic.mime(blob);

	if (meta.mime == "image/jpeg")
		if (auto exif = Exif::load_from_data(blob); exif)
			if (auto orientation = exif->orientation(); orientation)
				meta.orientation = *orientation;

	return meta;
}

void BlobDatabase::save_meta(const fs::path& dest_path, const Meta& meta) const
{
	std::ofstream file{(dest_path.parent_path() / std::string{metafile}).string()};
	rapidjson::OStreamWrapper osw{file};
	rapidjson::Writer<rapidjson::OStreamWrapper> writer{osw};

	meta.serialize().Accept(writer);
}

std::optional<BlobDatabase::Meta> BlobDatabase::load_meta(const ObjectID& id) const
{
	return load_meta(dest(id).parent_path() / std::string{metafile});
}

std::optional<BlobDatabase::Meta> BlobDatabase::load_meta(const fs::path& dest_path) const
{
	std::error_code ec;
	auto meta = MMap::open(dest_path, ec);
	if (ec)
		return std::nullopt;

	rapidjson::Document json;
	json.Parse(static_cast<const char*>(meta.data()), meta.size());

	Meta result;
	result.load(json);
	return result;
}

std::optional<std::string> BlobDatabase::load_meta_json(const ObjectID& id) const
{
	auto dest_path = dest(id);

	// Assume the size of the metadata file is small enough to fit in std::string
	std::error_code ec;
	auto mmap = MMap::open(dest_path.parent_path() / std::string{metafile}, ec);
	if (ec == std::errc::no_such_file_or_directory)
	{
		// Metafile not exists yet. This is not an error.
		// Just deduce it from the blob
		auto blob = MMap::open(dest_path, ec);
		if (ec)
		{
			Log(LOG_WARNING, "cannot open blob %1% (%2% %3%)", dest_path, ec, ec.message());
			return std::nullopt;
		}
		save_meta(dest_path, deduce_meta(blob.blob()));

		mmap = MMap::open(dest_path.parent_path() / std::string{metafile}, ec);
	}

	if (ec)
	{
		Log(LOG_WARNING, "cannot open metadata file for blob %1% (%2% %3%)", id, ec, ec.message());
		return std::nullopt;
	}

	return std::string{mmap.string()};
}

rapidjson::Document BlobDatabase::Meta::serialize() const
{
	rapidjson::Document json;
	json.SetObject();
	json.AddMember("mime", rapidjson::StringRef(mime.data(), mime.size()), json.GetAllocator());

	if (mime == "image/jpeg")
		json.AddMember("orientation", orientation, json.GetAllocator());

	return json;
}

void BlobDatabase::Meta::load(rapidjson::Document& json)
{
	mime = GetValueByPointerWithDefault(json, "/mime", mime).GetString();
	orientation = GetValueByPointerWithDefault(json, "/orientation", orientation).GetInt();
}
} // end of namespace hrb
