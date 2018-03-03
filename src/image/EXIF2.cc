/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 1/Mar/18.
//

#include "EXIF2.hh"

#include <boost/endian/buffers.hpp>

#include <array>
#include <iostream>

namespace hrb {

using namespace boost::endian;

namespace {

template <typename T>
T to_native(T value, order end)
{
	if (end == order::little)
		return little_to_native(value);
	else if (end == order::big)
		return big_to_native(value);
	else
		return value;
}

template <typename T>
T from_native(T value, order end)
{
	if (end == order::little)
		return native_to_little(value);
	else if (end == order::big)
		return native_to_big(value);
	else
		return value;
}

using buffer_view = std::basic_string_view<unsigned char>;

template <typename POD>
void read(buffer_view& buffer, POD& out, std::error_code& error)
{
	if (buffer.size() > sizeof(out))
	{
		std::memcpy(&out, buffer.data(), sizeof(out));
		buffer.remove_prefix(sizeof(out));
		error = EXIF2::Error::ok;
		assert(!error);
	}
	else
		error = EXIF2::Error::too_small;
}

void find_app1(buffer_view& buffer, std::error_code& error)
{
	struct App
	{
		unsigned char marker[2];
		big_uint16_buf_at length;
	};

	std::size_t index = 2;

	while (true)
	{
		App seg{};
		read(buffer, seg, error);
		if (error) return;

		// invalid segment marker
		if (seg.marker[0] != 0xFF)
		{
			error = EXIF2::Error::invalid_header;
			break;
		}

		// length too short
		if (seg.length.value() <= 2)
		{
			error = EXIF2::Error::invalid_header;
			break;
		}

		// check if whole segment within the buffer
		auto seg_remain = seg.length.value() - 2U;
		if (buffer.size() < seg_remain)
		{
			error = EXIF2::Error::too_small;
			break;
		}

		// found APP1
		if (seg.marker[1] == 0xE1)
		{
			// limit the size to APP1 segment
			buffer = buffer_view{buffer.data(), seg_remain};
			error = EXIF2::Error::ok;
			break;
		}

		// found DQT/DHT/DRI/SOF, which must be after APP1 segment
		else if (seg.marker[1] == 0xDB || seg.marker[1] == 0xC4 || seg.marker[1] == 0xDD || seg.marker[1] == 0xC0)
		{
			// limit the size to APP1 segment
			error = EXIF2::Error::not_found;
			break;

		}
		buffer.remove_prefix(seg_remain);
	}
}

const unsigned char* read_EXIF_and_TIFF_header(buffer_view& buffer, order& byte_order, std::error_code& error)
{
	// "Exif" header
	std::array<unsigned char, 6> exif{};
	read(buffer, exif, error);
	if (error)
		return nullptr;

	if (exif[0] != 0x45 ||
		exif[1] != 0x78 ||
		exif[2] != 0x69 ||
		exif[3] != 0x66 ||
		exif[4] != 0 ||
		exif[5] != 0)
	{
		error = EXIF2::Error::invalid_header;
		return nullptr;
	}

	// TIFF header
	struct TIFF
	{
		char byte_order[2];
		std::uint16_t four_two;
		std::uint32_t ifd_offset;
	};

	// record the TIFF header to locate tag value by offset later
	auto tiff_start = buffer.data();

	TIFF tiff{};
	read(buffer, tiff, error);
	if (error)
		return tiff_start;

	// check byte order
	if (tiff.byte_order[0] == 0x49 && tiff.byte_order[1] == 0x49)
		byte_order = order::little;
	else if (tiff.byte_order[0] == 0x4D && tiff.byte_order[1] == 0x4D)
		byte_order = order::big;
	else
		return nullptr;

	if (to_native(tiff.four_two, byte_order) != 0x2A)
		return nullptr;

	tiff.ifd_offset = to_native(tiff.ifd_offset, byte_order);
	buffer.remove_prefix(tiff.ifd_offset - sizeof(TIFF));

	return tiff_start;
}

} // end of anonymous namespace

EXIF2::EXIF2(const unsigned char *jpeg, std::size_t size, std::error_code& error)
{
	if (jpeg[0] != 0xFF ||
		jpeg[1] != 0xD8 )
	{
		error = Error::invalid_header;
		return;
	}

	buffer_view buffer{jpeg+2, size-2};
	find_app1(buffer, error);
	if (error)
		return;

	auto tiff = read_EXIF_and_TIFF_header(buffer, m_byte_order, error);
	if (error)
		return;

	assert(tiff);
	m_tiff_offset = tiff-jpeg;
	assert(m_tiff_offset > 0);

	std::uint16_t tag_count{};
	read(buffer, tag_count, error);
	if (error)
		return;
	tag_count = hrb::to_native(tag_count, m_byte_order);

	for (std::uint16_t i = 0 ; i < tag_count ; i++)
	{
		// convert to non-const pointer
		auto offset = buffer.data() - jpeg;
		assert(offset > 0);

		Field tag{};
		read(buffer, tag, error);
		if (error)
			return;

		to_native(tag);
		m_tags.emplace(static_cast<Tag>(tag.tag), offset);
	}
}

std::optional<EXIF2::Field> EXIF2::get(const unsigned char *jpeg, Tag tag) const
{
	auto it = m_tags.find(tag);
	if (it != m_tags.end())
	{
		assert(it->second > 0);

		Field field{};
		std::memcpy(&field, jpeg+it->second, sizeof(field));
		return to_native(field);
	}
	return std::nullopt;
}

EXIF2::Field& EXIF2::to_native(EXIF2::Field& field) const
{
	field.tag = hrb::to_native(field.tag, m_byte_order);
	field.type = hrb::to_native(field.type, m_byte_order);
	field.count = hrb::to_native(field.count, m_byte_order);
	field.value_offset = hrb::to_native(field.value_offset, m_byte_order);
	return field;
}

bool EXIF2::set(unsigned char *jpeg, const Field& native) const
{
	auto it = m_tags.find(static_cast<Tag>(native.tag));
	if (it != m_tags.end())
	{
		assert(it->second > 0);

		Field ordered_field{};
		ordered_field.tag = hrb::from_native(native.tag, m_byte_order);
		ordered_field.type = hrb::from_native(native.type, m_byte_order);
		ordered_field.count = hrb::from_native(native.count, m_byte_order);
		ordered_field.value_offset = hrb::from_native(native.value_offset, m_byte_order);

		std::memcpy(jpeg + it->second, &ordered_field, sizeof(ordered_field));

		return true;
	}
	return false;
}

const std::error_category& EXIF2::error_category()
{
	struct cat : std::error_category
	{
		const char* name() const noexcept override {return "EXIF";}
		std::string message(int code) const override
		{
			switch (static_cast<Error>(code))
			{
				case Error::ok: return "Success";
				case Error::invalid_header: return "Invalid header";
				case Error::too_small: return "Not enough data";
				default: return "Unknown";
			}
		}
	};
	static const cat c{};
	return c;
}

std::error_code make_error_code(EXIF2::Error error)
{
	return std::error_code{static_cast<int>(error), EXIF2::error_category()};
}

} // end of namespace
