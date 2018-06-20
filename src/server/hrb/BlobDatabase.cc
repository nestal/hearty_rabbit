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

// Other dependencies in the same module
#include "BlobFile.hh"
#include "CollEntryDB.hh"
#include "UploadFile.hh"

// Other modules in hearty rabbit
#include "net/MMapResponseBody.hh"
#include "util/Configuration.hh"
#include "common/Escape.hh"
#include "util/Log.hh"
#include "util/Magic.hh"

namespace hrb {

BlobDatabase::BlobDatabase(const Configuration& cfg) : m_cfg{cfg}
{
	if (exists(m_cfg.blob_path()) && !is_directory(m_cfg.blob_path()))
		throw std::system_error(std::make_error_code(std::errc::file_exists));

	if (!exists(m_cfg.blob_path()))
		create_directories(m_cfg.blob_path());
}

void BlobDatabase::prepare_upload(UploadFile& result, std::error_code& ec) const
{
	boost::system::error_code err;

	result.open(m_cfg.blob_path().string().c_str(), err);
	if (err)
		ec.assign(err.value(), err.category());
}

BlobFile BlobDatabase::save(UploadFile&& tmp, std::error_code& ec)
{
	return BlobFile{std::move(tmp), dest(tmp.ID()), ec};
}

fs::path BlobDatabase::dest(const ObjectID& id, std::string_view) const
{
	auto hex = to_hex(id);
	assert(hex.size() > 2);

	return m_cfg.blob_path() / hex.substr(0, 2) / hex;
}

BlobDatabase::BlobResponse BlobDatabase::meta(const ObjectID& id, unsigned version) const
{
	BlobFile blob_obj{dest(id), id};
	auto mmap = blob_obj.meta();
	if (!mmap.is_opened())
		return BlobResponse{http::status::not_found, version};

	BlobResponse res{
		std::piecewise_construct,
		std::make_tuple(std::move(mmap)),
		std::make_tuple(http::status::ok, version)
	};
	res.set(http::field::content_type, "application/json");
	set_cache_control(res, id);
	return res;
}

BlobDatabase::BlobResponse BlobDatabase::response(
	const ObjectID& id,
	unsigned version,
	std::string_view etag,
	std::string_view rendition
) const
{
	// validate rendition string
	auto invalid = std::find_if(rendition.begin(), rendition.end(), [](char c)
	{
		return !std::isalpha(c) && !std::isdigit(c);
	});
	if (invalid != rendition.end())
		return http::response<MMapResponseBody>{http::status::bad_request, version};

	std::error_code ec;
	BlobFile blob_obj{dest(id), id};
	auto mmap = blob_obj.rendition(rendition, m_cfg.renditions(), ec);
	if (ec)
		return BlobResponse{http::status::not_found, version};

	// the mime type of the rendition may not be the same as the master rendition
	// (which is stored in the meta data), so we need to deduce it again here.
	auto mime = Magic::instance().mime(mmap.blob());

	// Advice the kernel that we only read the memory in one pass
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

BlobFile BlobDatabase::find(const ObjectID& id) const
{
	return {dest(id), id};
}

double BlobDatabase::compare(const ObjectID& id1, const ObjectID& id2) const
{
	auto bf1 = find(id1);
	auto bf2 = find(id2);
	return bf1.compare(bf2);
}

} // end of namespace hrb
