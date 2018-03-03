/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/26/18.
//

#include "BlobMeta.hh"

#include "util/Magic.hh"
#include "util/JsonHelper.hh"
//#include "util/Log.hh"

#include <rapidjson/pointer.h>

namespace hrb {

rapidjson::Document BlobMeta::serialize() const
{
	rapidjson::Document json;
	json.SetObject();
	json.AddMember("mime", rapidjson::StringRef(m_mime), json.GetAllocator());

//	if (m_mime == "image/jpeg")
//		json.AddMember("orientation", m_orientation, json.GetAllocator());

	if (!m_filename.empty())
		json.AddMember("filename", rapidjson::StringRef(m_filename), json.GetAllocator());

	return json;
}

BlobMeta BlobMeta::load(rapidjson::Document& json)
{
	BlobMeta meta;
	meta.m_mime         = GetValueByPointerWithDefault(json, "/mime",        meta.m_mime).GetString();
//	meta.m_orientation  = GetValueByPointerWithDefault(json, "/orientation", meta.m_orientation).GetInt();
	meta.m_filename     = GetValueByPointerWithDefault(json, "/mime",        meta.m_filename).GetString();
	return meta;
}

BlobMeta BlobMeta::deduce_meta(boost::asio::const_buffer blob, const Magic& magic)
{
	BlobMeta meta;
	meta.m_mime = magic.mime(blob);

/*	if (meta.m_mime == "image/jpeg")
	{
		auto jpeg = boost::asio::buffer_cast<const unsigned char*>(blob);

		std::error_code ec;
		EXIF2 exif2{jpeg, blob.size(), ec};
		if (!ec)
		{
			if (auto field = exif2.get(jpeg, EXIF2::Tag::orientation))
				meta.m_orientation = field->value_offset;
		}
	}*/
	return meta;
}

} // end of namespace hrb
