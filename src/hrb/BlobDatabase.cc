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
#include "BlobMeta.hh"
#include "RotateImage.hh"

#include "net/MMapResponseBody.hh"

#include "util/Log.hh"
#include "util/Magic.hh"

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

ObjectID BlobDatabase::save(const UploadFile& tmp, std::string_view filename, std::error_code& ec)
{
	auto id = tmp.ID();
	auto dest_path = dest(id);

	boost::system::error_code bec;
	if (!exists(dest_path.parent_path()))
		create_directories(dest_path.parent_path(), bec);
	if (bec)
		Log(LOG_WARNING, "create directory error %1% %2%", bec, bec.message());

	ec.assign(bec.value(), bec.category());

	auto meta = deduce_meta(tmp);
	meta.filename(filename);
	save_meta(dest_path, meta);

	Log(LOG_NOTICE, "upload image orientation %1%", meta.orientation());

	// creates a rendition of auto-rotated image if the orientation isn't 1
	if (meta.mime() == "image/jpeg" && meta.orientation() != 1)
	{
		auto mmap = MMap::open(tmp.native_handle(), ec);
		if (!ec)
		{
			RotateImage trnasform;
			trnasform.auto_rotate(mmap.data(), mmap.size(), dest_path.parent_path() / "rotated.jpeg", ec);

			// Auto-rotation error is not fatal.
			if (!ec)
			{
				Log(LOG_WARNING, "cannot rotate image %1%. Sizes not divisible by 16?", filename);
				ec.clear();
			}
		}
	}

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
		meta = BlobMeta::deduce_meta(mmap.blob(), m_magic);
		if (!meta)
		{
			Log(LOG_WARNING, "cannot load metadata from file %1%", path);
			meta.emplace(BlobMeta{});
		}

		save_meta(path.parent_path()/std::string{metafile}, *meta);
	}

	// Serve the rotated image if it exists
	if (meta->mime() == "image/jpeg" && meta->orientation() != 1 && exists(path.parent_path()/"rotated.jpeg"))
	{
		auto rotated = MMap::open(path.parent_path() / "rotated.jpeg", ec);
		if (!ec)
			mmap = std::move(rotated);
	}

	// Advice the kernel that we only read the memory in one pass
	mmap.cache();

	BlobResponse res{
		std::piecewise_construct,
		std::make_tuple(std::move(mmap)),
		std::make_tuple(http::status::ok, version)
	};
	res.set(http::field::content_type, meta->mime());
	set_cache_control(res, id);
	return res;

}

void BlobDatabase::set_cache_control(BlobResponse& res, const ObjectID& id)
{
	res.set(http::field::cache_control, "private, max-age=31536000, immutable");
	res.set(http::field::etag, to_quoted_hex(id));
}

BlobMeta BlobDatabase::deduce_meta(const UploadFile& tmp) const
{
	// Deduce mime type and orientation (if it is a JPEG)
	std::error_code ec;
	auto mmap = MMap::open(tmp.native_handle(), ec);
	if (ec)
	{
		Log(LOG_WARNING, "cannot open tmp file to deduce meta");
		return {};
	}

	return BlobMeta::deduce_meta(mmap.blob(), m_magic);
}

void BlobDatabase::save_meta(const fs::path& dest_path, const BlobMeta& meta) const
{
	std::ofstream file{(dest_path.parent_path() / std::string{metafile}).string()};
	rapidjson::OStreamWrapper osw{file};
	rapidjson::Writer<rapidjson::OStreamWrapper> writer{osw};

	meta.serialize().Accept(writer);
}

std::optional<BlobMeta> BlobDatabase::load_meta(const ObjectID& id) const
{
	return load_meta(dest(id).parent_path() / std::string{metafile});
}

std::optional<BlobMeta> BlobDatabase::load_meta(const fs::path& dest_path) const
{
	std::error_code ec;
	auto meta = MMap::open(dest_path, ec);
	if (ec)
		return std::nullopt;

	rapidjson::Document json;
	json.Parse(static_cast<const char*>(meta.data()), meta.size());

	return BlobMeta::load(json);
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
		save_meta(dest_path, BlobMeta::deduce_meta(blob.blob(), m_magic));

		mmap = MMap::open(dest_path.parent_path() / std::string{metafile}, ec);
	}

	if (ec)
	{
		Log(LOG_WARNING, "cannot open metadata file for blob %1% (%2% %3%)", id, ec, ec.message());
		return std::nullopt;
	}

	return std::string{mmap.string()};
}

} // end of namespace hrb
