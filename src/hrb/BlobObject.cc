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
#include "UploadFile.hh"

#include "image/RotateImage.hh"
#include "image/JPEG.hh"
#include "util/MMap.hh"
#include "util/Log.hh"

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

		result.m_rotated = std::move(rotated);
	}

	result.m_master = std::move(master);
	result.m_meta   = std::move(meta);
	result.m_id     = tmp.ID();

	return result;
}

BufferView BlobObject::blob() const
{
	return m_master.buffer();
}

} // end of namespace hrb
