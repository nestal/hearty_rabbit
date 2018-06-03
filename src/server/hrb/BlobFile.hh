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

#include "common/ObjectID.hh"
#include "common/Timestamp.hh"

#include "image/PHash.hh"
#include "util/Size2D.hh"
#include "util/FS.hh"
#include "util/MMap.hh"

#include <system_error>
#include <chrono>

namespace hrb {

class RenditionSetting;
class JPEGRenditionSetting;
class UploadFile;

/// \brief  On-disk representation of a blob
/// A blob is an object that is stored in hearty rabbit. BlobFile represents how it
/// is stored in the file system. This class does not care about how blobs are stored
/// in redis. It only cares about the file system.
///
/// Each blob is stored in its own directory. BlobFile does not know how to find that
/// directory by the blob ID. It is given a directory during construction. BlobFile
/// is responsible for managing this directory. It generates the renditions when they
/// are required and save them in the directory.
///
/// BlobDatabase is responsible for assigning different directories to different blobs.
class BlobFile
{
public:
	BlobFile() = default;
	BlobFile(const fs::path& dir, const ObjectID& id);
	BlobFile(UploadFile&& tmp, const fs::path& dir, std::error_code& ec);

	// if the rendition does not exists but it's a valid one, it will be generated dynamically
	MMap rendition(std::string_view rendition, const RenditionSetting& cfg, std::error_code& ec) const;
	MMap master(std::error_code& ec) const;

	const ObjectID& ID() const {return m_id;}
	std::string_view mime() const;
	std::optional<PHash> phash() const;

	Timestamp original_datetime() const;

	bool is_image() const;
	double compare(const BlobFile& other) const;

private:
	static bool is_image(std::string_view mime);
	void generate_image_rendition(const JPEGRenditionSetting& cfg, const fs::path& dest, std::error_code& ec) const;
	void update_meta() const;
	MMap deduce_meta(MMap&& master) const;
	MMap deduce_mime(MMap&& master) const;
	MMap deduce_phash(MMap&& master) const;
	MMap deduce_original(MMap&& master) const;
	MMap deduce_uploaded(MMap&& master) const;
	MMap master(MMap&& master) const;

private:
	ObjectID    m_id{};				//!< ID of the blob
	fs::path    m_dir;				//!< The directory in file system that stores all renditions of the blob
	struct Meta
	{
		std::string	mime;						//!< Mime type of the master rendition
		std::optional<PHash> 		phash;		//!< Phash of the master rendition (for images only)
		std::optional<Timestamp>	original;	//!< Date time stored inside the master rendition (e.g. EXIF2)
		Timestamp	uploaded;
	};
	mutable std::optional<Meta>	m_meta;
};

} // end of namespace hrb