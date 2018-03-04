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

#include <rapidjson/pointer.h>

namespace hrb {

rapidjson::Document BlobMeta::serialize() const
{
	rapidjson::Document json;
	json.SetObject();
	json.AddMember("mime", rapidjson::StringRef(m_mime), json.GetAllocator());

	if (!m_filename.empty())
		json.AddMember("filename", rapidjson::StringRef(m_filename), json.GetAllocator());

	return json;
}

BlobMeta BlobMeta::load(rapidjson::Document& json)
{
	BlobMeta meta;
	meta.m_mime         = GetValueByPointerWithDefault(json, "/mime",     meta.m_mime).GetString();
	meta.m_filename     = GetValueByPointerWithDefault(json, "/filename", meta.m_filename).GetString();
	return meta;
}

BlobMeta BlobMeta::deduce_meta(boost::asio::const_buffer blob, const Magic& magic)
{
	BlobMeta meta;
	meta.m_mime = magic.mime(blob);
	return meta;
}

} // end of namespace hrb
