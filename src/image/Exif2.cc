/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 1/Mar/18.
//

#include "Exif2.hh"

#include <boost/endian/buffers.hpp>

#include <iostream>
#include <array>

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

template <typename POD>
bool read(std::string_view& buffer, POD& out)
{
	if (buffer.size() > sizeof(out))
	{
		std::memcpy(&out, buffer.data(), sizeof(out));
		buffer.remove_prefix(sizeof(out));
		return true;
	}
	return false;
}

bool find_app1(std::string_view& buffer)
{
	struct App
	{
		unsigned char marker[2];
		big_uint16_buf_at length;
	};

	std::size_t index = 2;

	while (true)
	{
		App seg;
		if (!read(buffer, seg))
			return false;

		std::cout << (int)seg.marker[0] << " " << (int)seg.marker[1] << " length = " << seg.length.value() << std::endl;

		// invalid segment marker
		if (seg.marker[0] != 0xFF) return false;

		// length too short
		if (seg.length.value() <= 2) return false;

		// check if whole segment within the buffer
		auto seg_remain = seg.length.value() - 2U;
		if (buffer.size() < seg_remain) return false;

		// found APP1
		if (seg.marker[1] == 0xE1)
		{
			// limit the size to APP1 segment
			buffer = std::string_view{buffer.data(), seg_remain};
			return true;
		}
		buffer.remove_prefix(seg_remain);
	}
}

bool read_EXIF_and_TIFF_header(std::string_view& buffer, order& byte_order)
{
	// "Exif" header
	std::array<unsigned char, 6> exif;
	if (!read(buffer, exif)) return false;
	if (exif[0] != 0x45 ||
		exif[1] != 0x78 ||
		exif[2] != 0x69 ||
		exif[3] != 0x66 ||
		exif[4] != 0 ||
		exif[5] != 0)
		return false;

	// TIFF header
	struct TIFF
	{
		char byte_order[2];
		std::uint16_t four_two;
		std::uint32_t ifd_offset;
	} tiff;
	if (!read(buffer, tiff)) return false;

	// check byte order
	if (tiff.byte_order[0] == 0x49 && tiff.byte_order[1] == 0x49)
		byte_order = order::little;
	else if (tiff.byte_order[0] == 0x4D && tiff.byte_order[1] == 0x4D)
		byte_order = order::big;
	else
		return false;

	if (to_native(tiff.four_two, byte_order) != 0x2A)
		return false;

	tiff.ifd_offset = to_native(tiff.ifd_offset, byte_order);
	buffer.remove_prefix(tiff.ifd_offset - sizeof(TIFF));

	return true;
}

} // end of anonymous namespace

Exif2::Exif2(unsigned char *jpeg, std::size_t size)
{
	if (jpeg[0] != 0xFF ||
		jpeg[1] != 0xD8 )
	{
		std::cout << "invalid header" << std::endl;
		return;
	}

	std::string_view buffer{reinterpret_cast<const char*>(jpeg+2), size-2};
	if (!find_app1(buffer))
		return;

	if (!read_EXIF_and_TIFF_header(buffer, m_byte_order))
		return;

	if (size < 2)
		return;

	std::cout << "everything OK so far" << std::endl;
	std::uint16_t tag_count{};
	if (!read(buffer, tag_count)) return;
	tag_count = hrb::to_native(tag_count, m_byte_order);

	std::cout << tag_count << " tags in file " << std::endl;

	for (std::uint16_t i = 0 ; i < tag_count ; i++)
	{
		auto ptags = jpeg + (reinterpret_cast<const unsigned char*>(buffer.data()) - jpeg);

		IFD tag;
		if (!read(buffer, tag)) return;

		to_native(tag);
		m_tags.emplace(tag.tag, ptags);

		std::cout << "tag " << std::hex << tag.tag << std::dec << " " << tag.type << " " << tag.count << " " << tag.value_offset << std::endl;
	}
}

std::optional<Exif2::IFD> Exif2::get(std::uint16_t tag) const
{
	auto it = m_tags.find(tag);
	if (it != m_tags.end())
	{
		IFD field{};
		std::memcpy(&field, it->second, sizeof(field));
		to_native(field);
		return field;
	}
	return std::nullopt;
}

void Exif2::to_native(Exif2::IFD& field) const
{
	field.tag = hrb::to_native(field.tag, m_byte_order);
	field.type = hrb::to_native(field.type, m_byte_order);
	field.count = hrb::to_native(field.count, m_byte_order);
	field.value_offset = hrb::to_native(field.value_offset, m_byte_order);
}

} // end of namespace
