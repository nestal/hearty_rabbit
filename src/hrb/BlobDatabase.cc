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

#include <sstream>

namespace hrb {
namespace {
const std::string_view default_rendition = "master";
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

fs::path BlobDatabase::dest(ObjectID id, std::string_view) const
{
	auto hex = to_hex(id);
	assert(hex.size() > 2);

	return m_base / hex.substr(0, 2) / hex / std::string{default_rendition};
}

BlobDatabase::BlobResponse BlobDatabase::response(
	ObjectID id,
	const Magic& magic,
	unsigned version,
	std::string_view rendition
) const
{
	auto path = dest(id);

	std::error_code ec;
	auto mmap = MMap::open(path, ec);
	if (ec)
		return BlobResponse{http::status::not_found, version};

	auto mime = magic.mime(mmap.blob());

	BlobResponse res{
		std::piecewise_construct,
		std::make_tuple(std::move(mmap)),
		std::make_tuple(http::status::ok, version)
	};
	res.set(http::field::content_type, mime);
	res.set(http::field::cache_control, "private, max-age=0, must-revalidate");
	res.set(http::field::etag, to_hex(id));
	res.prepare_payload();
	return res;

}

} // end of namespace hrb
