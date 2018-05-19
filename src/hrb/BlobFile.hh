/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/3/18.
//

#pragma once

#include "ObjectID.hh"
#include "UploadFile.hh"

#include "image/TurboBuffer.hh"
#include "image/PHash.hh"
#include "util/Size2D.hh"
#include "util/FS.hh"
#include "util/MMap.hh"

#include <system_error>

namespace hrb {

class EXIF2;
class Magic;
class RenditionSetting;
class JPEGRenditionSetting;
class UploadFile;

class BlobFile
{
public:
	BlobFile() = default;
	BlobFile(const fs::path& dir, const ObjectID& id);
	BlobFile(
		UploadFile&& tmp,
		const fs::path& dir,
		const Magic& magic,
		std::error_code& ec
	);

	MMap rendition(std::string_view rendition, const RenditionSetting& cfg, std::error_code& ec) const;

	const ObjectID& ID() const {return m_id;}
	std::string mime() const {return m_mime;}

	auto phash() const {return m_phash;}

private:
	static TurboBuffer generate_rendition_from_jpeg(BufferView jpeg_master, Size2D dim, int quality, std::error_code& ec);

private:
	ObjectID    m_id{};
	fs::path    m_dir;
	std::string m_mime;
	PHash       m_phash{};
};

} // end of namespace hrb
