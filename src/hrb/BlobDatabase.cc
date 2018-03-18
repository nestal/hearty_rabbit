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
#include "CollEntry.hh"
#include "BlobFile.hh"

#include "net/MMapResponseBody.hh"

#include "util/Log.hh"

namespace hrb {

BlobDatabase::BlobDatabase(const fs::path& base, const Size2D& img_resize) : m_base{base}, m_resize_img{img_resize}
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

BlobFile BlobDatabase::save(UploadFile&& tmp, std::string_view filename, std::error_code& ec)
{
	auto blob_obj = BlobFile::upload(std::move(tmp), m_magic, m_resize_img, filename, 70, ec);
	if (!ec)
		blob_obj.save(dest(blob_obj.ID()), ec);

	return blob_obj;
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
	std::string_view mime,
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
	BlobFile blob_obj{path, id, m_resize_img, ec};

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
	res.set(http::field::content_type, mime);
	set_cache_control(res, id);
	return res;

}

void BlobDatabase::set_cache_control(BlobResponse& res, const ObjectID& id)
{
	res.set(http::field::cache_control, "private, max-age=31536000, immutable");
	res.set(http::field::etag, to_quoted_hex(id));
}

} // end of namespace hrb
