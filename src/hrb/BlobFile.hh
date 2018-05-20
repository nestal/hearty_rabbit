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

#include "image/PHash.hh"
#include "util/Size2D.hh"
#include "util/FS.hh"
#include "util/MMap.hh"

#include <system_error>

namespace hrb {

class EXIF2;
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
		std::error_code& ec
	);

	// if the rendition does not exists but it's a valid one, it will be generated dynamically
	MMap rendition(std::string_view rendition, const RenditionSetting& cfg, std::error_code& ec) const;
	MMap master(std::error_code& ec) const;

	const ObjectID& ID() const {return m_id;}
	std::string mime() const {return m_mime;}
	auto phash() const {return m_phash;}

	bool is_image() const;

private:
	void generate_image_rendition(const JPEGRenditionSetting& cfg, const fs::path& dest, std::error_code& ec) const;

private:
	ObjectID    m_id{};     //!< ID of the blob
	fs::path    m_dir;      //!< The directory in file system that stores all renditions of the blob
	mutable std::string  m_mime;     //!< Mime type of the master rendition
	std::optional<PHash> m_phash;
};

} // end of namespace hrb
