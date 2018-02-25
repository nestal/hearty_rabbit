/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/25/18.
//

#pragma once

#include "FS.hh"

#include <libexif/exif-data.h>
#include <chrono>
#include <optional>

namespace hrb {

// Simple EXIF wrapper
class Exif
{
public:
	explicit Exif(const fs::path& path);
	Exif(const void *data, std::size_t size);
	Exif(const Exif&) = delete;
	Exif(Exif&&) = default;
	~Exif();

	Exif& operator=(const Exif&) = delete;
	Exif& operator=(Exif&&) = default;

	std::optional<int> orientation() const;

	int ISO() const;
	std::chrono::system_clock::time_point datetime() const;
	std::string datetime_str() const;

private:
	ExifData    *m_exif{};
};

} // end of namespace hrb
