/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/28/18.
//

#pragma once

#include "PHash.hh"

#include "util/BufferView.hh"
#include "util/Timestamp.hh"

#include <nlohmann/json.hpp>

#include <opencv2/core.hpp>

#include <optional>

namespace hrb {

cv::Mat load_image(BufferView raw);

class ImageMeta
{
public:
	ImageMeta() = default;
	explicit ImageMeta(BufferView master);

	[[nodiscard]] auto& mime() const {return m_mime;}
	[[nodiscard]] auto& phash() const {return m_phash;}
	[[nodiscard]] auto& original_timestamp() const {return m_original;}
	[[nodiscard]] auto& upload_timestamp() const {return m_uploaded;}

	friend void from_json(const nlohmann::json& src, ImageMeta& dest);
	friend void to_json(nlohmann::json& dest, const ImageMeta& src);

private:
	std::string	                m_mime;			//!< Mime type of the master rendition
	std::optional<PHash> 		m_phash;		//!< Phash of the master rendition (for images only)
	std::optional<Timestamp>	m_original;		//!< Date time stored inside the master rendition (e.g. EXIF2)
	Timestamp	                m_uploaded;
};

} // end of namespace hrb
