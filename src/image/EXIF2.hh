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

#include <boost/endian/conversion.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <optional>
#include <system_error>

namespace hrb {

class EXIF2
{
public:
	struct IFD
	{
		std::uint16_t tag;
		std::uint16_t type;
		std::uint32_t count;
		std::uint32_t value_offset;
	};

	enum class Error {ok, too_small, invalid_header};

	static const std::error_category& error_category();

public:
	EXIF2() = default;
	EXIF2(unsigned char *jpeg, std::size_t size, std::error_code& ec);

	std::optional<IFD> get(std::uint16_t tag) const;
	bool set(const IFD& native);

private:
	IFD& to_native(IFD& field) const;

private:
	unsigned char *m_tiff_start{};

	std::unordered_map<std::uint16_t, unsigned char*> m_tags;
	boost::endian::order m_byte_order{};
};

std::error_code make_error_code(EXIF2::Error error);

} // end of namespace

namespace std
{
	template <> struct is_error_code_enum<hrb::EXIF2::Error> : true_type {};
}
