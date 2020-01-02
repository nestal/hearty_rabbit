/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 1/Mar/18.
//

#pragma once

#include "common/BufferView.hh"

// libexif to read EXIF2 tags
#include <libexif/exif-data.h>

#include <memory>
#include <optional>
#include <chrono>

namespace hrb {

class EXIF2
{
public:
	explicit EXIF2(BufferView jpeg);
	EXIF2(EXIF2&&) = default;
	EXIF2(const EXIF2&) = delete;
	~EXIF2();

	EXIF2& operator=(EXIF2&&) = default;
	EXIF2& operator=(const EXIF2&) = delete;

	[[nodiscard]] explicit operator bool() const noexcept {return m_data != nullptr;}

	[[nodiscard]] std::optional<std::chrono::system_clock::time_point> date_time() const;

private:
	// Use unique_ptr to ensure the ExifData will be freed.
	struct Unref
	{
		void operator()(::ExifData*) const;
	};
	std::unique_ptr<::ExifData, Unref>  m_data;
};

} // end of namespace
