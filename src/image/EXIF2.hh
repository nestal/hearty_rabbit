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

#include "util/BufferView.hh"

#include <boost/endian/conversion.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <optional>
#include <system_error>

namespace hrb {

/// Simple read/write access to EXIF2 tags in JPEG images.
/// This class provides a very simple read/write access to EXIF2 tags in JPEG images.
/// The primary purpose is to extract the orientation of the image, rotate it accordingly
/// and reset the value. In other words, perform similar to ``jhead -autorot''.
///
/// Reference:
/// http://www.exif.org/Exif2-2.PDF
/// https://www.media.mit.edu/pia/Research/deepview/exif.html
/// http://vip.sugovica.hu/Sardi/kepnezo/JPEG%20File%20Layout%20and%20Format.htm
class EXIF2
{
public:
	struct Field
	{
		std::uint16_t tag;
		std::uint16_t type;
		std::uint32_t count;
		std::uint32_t value_offset;
	};

	enum class Error {ok, too_small, invalid_header, not_found, not_supported};

	enum class Tag {
		orientation = 0x112,
		date_time   = 0x132,
		exif_offset = 0x8769,
		exif_version = 0x9000,
		date_time_original = 0x9003,
		date_time_digitalized = 0x9004,
		color_space = 0xa001
	};

	static const std::error_category& error_category();

public:
	EXIF2() = default;
	EXIF2(const unsigned char *jpeg, std::size_t size, std::error_code& ec);
	EXIF2(BufferView blob, std::error_code& ec);

	std::optional<Field> get(const unsigned char *jpeg, Tag tag) const;
	bool set(unsigned char *jpeg, const Field& native) const;

private:
	Field& to_native(Field& field) const;

	void read_ifd(const unsigned char* jpeg, BufferView& buffer, std::error_code& error);

private:
	std::ptrdiff_t m_tiff_offset{};

	std::unordered_map<Tag, std::ptrdiff_t> m_tags;
	boost::endian::order m_byte_order{};
};

std::error_code make_error_code(EXIF2::Error error);

} // end of namespace

namespace std
{
	template <> struct is_error_code_enum<hrb::EXIF2::Error> : true_type {};
}
