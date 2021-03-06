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

#include "hrb/ObjectID.hh"
#include "util/Timestamp.hh"

#include "image/Image.hh"
#include "util/Size2D.hh"
#include "util/FS.hh"
#include "util/MMap.hh"

#include <nlohmann/json.hpp>
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
	MMap rendition(std::string_view rendition, const RenditionSetting& cfg, const fs::path& haar_path, std::error_code& ec) const;
	MMap load_master(std::error_code& ec) const;

	const ObjectID& ID() const {return m_id;}
	std::string_view mime() const;
	std::optional<PHash> phash() const;

	nlohmann::json meta_json() const;
	MMap load_meta() const;
	auto& meta() const {return m_meta;}

	Timestamp original_datetime() const;

	bool is_image() const;
	double compare(const BlobFile& other) const;

private:
	static bool is_image(std::string_view mime);
	void generate_image_rendition(const JPEGRenditionSetting& cfg, const fs::path& dest, const fs::path& haar_path, std::error_code& ec) const;
	void update_meta() const;
	MMap deduce_meta(MMap&& master) const;
	MMap master(MMap&& master) const;

private:
	ObjectID    m_id{};				//!< ID of the blob
	fs::path    m_dir;				//!< The directory in file system that stores all renditions of the blob

	mutable std::optional<ImageMeta>	m_meta;
};

} // end of namespace hrb
