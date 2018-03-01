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

public:
	EXIF2(unsigned char *jpeg, std::size_t size);

	std::optional<IFD> get(std::uint16_t tag) const;
	void set(const IFD& native);

private:
	void to_native(IFD& field) const;

private:
	std::unordered_map<std::uint16_t, unsigned char*> m_tags;
	boost::endian::order m_byte_order{};
};

} // end o namespace
