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

#include <string_view>
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

Exif2::Exif2(const unsigned char *jpeg, std::size_t size)
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

	order byte_order{order::native};
	if (!read_EXIF_and_TIFF_header(buffer, byte_order))
		return;

	if (size < 2)
		return;

	std::cout << "everything OK so far" << std::endl;
	std::uint16_t tag_count{};
	if (!read(buffer, tag_count)) return;
	tag_count = to_native(tag_count, byte_order);

	std::cout << tag_count << " tags in file " << std::endl;

	struct IFD
	{
		std::uint16_t tag;
		std::uint16_t type;
		std::uint32_t count;
		std::uint32_t value_offset;
	};
	for (std::uint16_t i = 0 ; i < tag_count ; i++)
	{
		IFD tag;
		if (!read(buffer, tag)) return;

		tag.tag = to_native(tag.tag, byte_order);
		tag.type = to_native(tag.type, byte_order);
		tag.count = to_native(tag.count, byte_order);
		tag.value_offset = to_native(tag.value_offset, byte_order);

		std::cout << "tag " << std::hex << tag.tag << std::dec << " " << tag.type << " " << tag.count << " " << tag.value_offset << std::endl;
	}
}

} // end of namespace
