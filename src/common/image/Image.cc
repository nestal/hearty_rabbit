/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/28/18.
//

#include "Image.hh"
#include "util/Magic.hh"
#include "EXIF2.hh"

#include <opencv2/imgcodecs.hpp>

namespace hrb {

cv::Mat load_image(BufferView raw)
{
	return cv::imdecode(
		cv::Mat{1, static_cast<int>(raw.size()), CV_8U, const_cast<unsigned char*>(raw.data())},
		cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH
	);
}


void to_json(nlohmann::json& dest, const ImageMeta& src)
{
	// save the meta data to file
	using namespace std::chrono;
	nlohmann::json meta{
		{"mime", src.m_mime}
	};

	// just to be sure
	assert(meta.is_object());

	if (src.m_original)
		meta.emplace("original_datetime", *src.m_original);
	if (src.m_phash)
		meta.emplace("phash", src.m_phash->value());

	dest = std::move(meta);
}

void from_json(const nlohmann::json& src, ImageMeta& dest)
{
	if (src.is_object())
	{
		dest.m_mime = src["mime"];

		if (src.count("original_datetime") > 0)
			dest.m_original = src["original_datetime"].get<Timestamp>();
		else
			dest.m_original = std::nullopt;

		if (src.count("phash") > 0)
			dest.m_phash = PHash{src["phash"].get<std::uint64_t>()};
		else
			dest.m_phash = std::nullopt;
	}
}

ImageMeta::ImageMeta(BufferView master) :
	m_mime{Magic::instance().mime(master)},
	m_phash{std::invoke(
		[](BufferView master) -> decltype(m_phash)
		{
			try
			{
				return hrb::phash(master);
			}
			catch (...)
			{
				return std::nullopt;
			}
		},
		master
	)
}
{
	if (m_mime == "image/jpeg")
	{
		if (EXIF2 exif{master}; exif)
			if (auto datetime = exif.date_time(); datetime)
				m_original = std::chrono::time_point_cast<Timestamp::duration>(*datetime);
	}

	if (!m_original)
		m_original = std::chrono::time_point_cast<Timestamp::duration>(Timestamp::clock::now());

	// not sure if this is right
	m_uploaded = *m_original;
}

} // end of namespace hrb
