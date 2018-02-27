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
#include "util/Log.hh"

#include <turbojpeg.h>
#include <exiv2/exiv2.hpp>

#include <rapidjson/pointer.h>

namespace hrb {

rapidjson::Document BlobMeta::serialize() const
{
	rapidjson::Document json;
	json.SetObject();
	json.AddMember("mime", rapidjson::StringRef(m_mime), json.GetAllocator());

	if (m_mime == "image/jpeg")
		json.AddMember("orientation", m_orientation, json.GetAllocator());

	if (!m_filename.empty())
		json.AddMember("filename", rapidjson::StringRef(m_filename), json.GetAllocator());

	return json;
}

BlobMeta BlobMeta::load(rapidjson::Document& json)
{
	BlobMeta meta;
	meta.m_mime         = GetValueByPointerWithDefault(json, "/mime",        meta.m_mime).GetString();
	meta.m_orientation  = GetValueByPointerWithDefault(json, "/orientation", meta.m_orientation).GetInt();
	meta.m_filename     = GetValueByPointerWithDefault(json, "/mime",        meta.m_filename).GetString();
	return meta;
}

BlobMeta BlobMeta::deduce_meta(boost::asio::const_buffer blob, const Magic& magic)
{
	BlobMeta meta;
	meta.m_mime = magic.mime(blob);

	if (meta.m_mime == "image/jpeg")
	{
		auto ev2 = Exiv2::ImageFactory::open(static_cast<const unsigned char*>(blob.data()), blob.size());
	    ev2->readMetadata();
		auto& exif = ev2->exifData();
		auto orientation = exif.findKey(Exiv2::ExifKey{"Exif.Image.Orientation"});
		if (orientation != exif.end())
			meta.m_orientation = orientation->toLong();
/*		if (auto exif = Exif::load_from_data(blob))
		{
			if (auto orientation = exif->orientation())
				meta.m_orientation = *orientation;

			if (auto doc_name = exif->document_name())
			{
				Log(LOG_NOTICE, "detected filename %1%", *doc_name);
				meta.m_filename = std::move(*doc_name);
			}
		}*/
	}
	return meta;
}

} // end of namespace hrb
