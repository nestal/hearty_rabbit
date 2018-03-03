/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/3/18.
//

#include "BlobObject.hh"
#include "BlobMeta.hh"

#include "image/RotateImage.hh"
#include "image/JPEG.hh"
#include "util/MMap.hh"
#include "util/Log.hh"

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/pointer.h>

namespace hrb {

BlobObject::BlobObject(const fs::path& base, const ObjectID& id) : m_id{id}
{

}

const BlobMeta& BlobObject::meta() const
{
	return m_meta;
}

BlobObject BlobObject::upload(UploadFile&& tmp, const Magic& magic, const Size& resize_img, std::error_code& ec)
{
	BlobObject result;

	auto master = MMap::open(tmp.native_handle(), ec);
	if (ec)
		return result;

	auto meta = BlobMeta::deduce_meta(master.blob(), magic);
	if (meta.mime() == "image/jpeg")
	{
		RotateImage transform;
		auto rotated = transform.auto_rotate(master.data(), master.size(), ec);
		if (ec)
		{
			Log(LOG_WARNING, "BlobDatabase::resize(): cannot rotate image %1% %2%", ec, ec.message());
			return result;
		}

		JPEG img{
			rotated.empty() ? static_cast<const unsigned char*>(master.data()) : rotated.data(),
			rotated.empty() ? master.size()                                    : rotated.size(),
			resize_img
		};

		if (resize_img != img.size())
			rotated = img.compress(70);

//		result.m_rotated = std::move(rotated);
		std::ostringstream fn;
		fn << resize_img.width() << "x" << resize_img.height();

		result.m_rend.emplace(fn.str(), std::move(rotated));
	}

	// commit result
	result.m_id     = tmp.ID();
	result.m_tmp    = std::move(tmp);
	result.m_master = std::move(master);
	result.m_meta   = std::move(meta);

	return result;
}

BufferView BlobObject::blob() const
{
	return m_master.buffer();
}

template <typename Blob>
void save_blob(const Blob& blob, const fs::path& dest, std::error_code& ec)
{
	boost::system::error_code bec;
	boost::beast::file file;
	file.open(dest.string().c_str(), boost::beast::file_mode::write, bec);
	file.write(blob.data(), blob.size(), bec);
	if (bec)
		ec.assign(bec.value(), bec.category());
	else
		ec.assign(0, ec.category());
}

void BlobObject::save(const fs::path& dir, std::error_code& ec) const
{
	// Try moving the temp file to our destination first. If failed, use
	// deep copy instead.
	m_tmp.move(dir / "master", ec);
	if (ec)
		save_blob(m_master, dir/"master", ec);

	// Save the renditions, if any.
	for (auto&& [name, rend] : m_rend)
	{
		save_blob(rend, dir / name, ec);
		if (ec)
			break;
	}

	// Save meta data
	std::ofstream file{(dir / "meta").string()};
	rapidjson::OStreamWrapper osw{file};
	rapidjson::Writer<rapidjson::OStreamWrapper> writer{osw};

	m_meta.serialize().Accept(writer);
}

} // end of namespace hrb
