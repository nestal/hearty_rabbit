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
#include "BlobObject.hh"

#include "net/MMapResponseBody.hh"

#include "util/Log.hh"

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/pointer.h>

namespace hrb {

BlobDatabase::BlobDatabase(const fs::path& base, const Size& img_resize) : m_base{base}, m_resize_img{img_resize}
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

ObjectID BlobDatabase::save(UploadFile&& tmp, std::string_view filename, std::error_code& ec)
{
	auto blob_obj = BlobObject::upload(std::move(tmp), m_magic, m_resize_img, filename, ec);
	if (!ec)
		blob_obj.save(dest(blob_obj.ID()), ec);

	return blob_obj.ID();
}

fs::path BlobDatabase::dest(const ObjectID& id, std::string_view) const
{
	auto hex = to_hex(id);
	assert(hex.size() > 2);

	return m_base / hex.substr(0, 2) / hex;
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
	BlobObject blob_obj{path, id, m_resize_img, ec};

	if (ec)
		return BlobResponse{http::status::not_found, version};

	// Advice the kernel that we only read the memory in one pass
	auto mmap{std::move(blob_obj.master())};
	mmap.cache();

	BlobResponse res{
		std::piecewise_construct,
		std::make_tuple(std::move(mmap)),
		std::make_tuple(http::status::ok, version)
	};
	res.set(http::field::content_type, blob_obj.meta().mime());
	set_cache_control(res, id);
	return res;

}

void BlobDatabase::set_cache_control(BlobResponse& res, const ObjectID& id)
{
	res.set(http::field::cache_control, "private, max-age=31536000, immutable");
	res.set(http::field::etag, to_quoted_hex(id));
}

void BlobDatabase::save_meta(const fs::path& dest_path, const BlobMeta& meta) const
{
	std::ofstream file{(dest_path.parent_path() / std::string{"meta"}).string()};
	rapidjson::OStreamWrapper osw{file};
	rapidjson::Writer<rapidjson::OStreamWrapper> writer{osw};

	meta.serialize().Accept(writer);
}

std::optional<std::string> BlobDatabase::load_meta_json(const ObjectID& id) const
{
	auto dest_path = dest(id);

	// Assume the size of the metadata file is small enough to fit in std::string
	std::error_code ec;
	auto mmap = MMap::open(dest_path / std::string{"meta"}, ec);
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

		mmap = MMap::open(dest_path.parent_path() / std::string{"meta"}, ec);
	}

	if (ec)
	{
		Log(LOG_WARNING, "cannot open metadata file for blob %1% (%2% %3%)", id, ec, ec.message());
		return std::nullopt;
	}

	return std::string{mmap.string()};
}

} // end of namespace hrb
