/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/15/18.
//

#include "ObjectID.hh"

#include "common/util/Blake2.hh"
#include "common/util/Escape.hh"

#include <boost/algorithm/hex.hpp>

#include <cstring>
#include <cassert>
#include <fstream>
#include <iomanip>

namespace hrb {

ObjectID::ObjectID(const std::array<unsigned char, Blake2::size>& array)
{
	std::copy(array.begin(), array.end(), begin());
}

std::optional<ObjectID> ObjectID::from_json(const nlohmann::json& json)
{
	return ObjectID::from_hex(json.get<std::string>());
}

std::optional<ObjectID> ObjectID::from_hex(std::string_view hex)
{
	auto opt_array = hex_to_array<ObjectID{}.size()>(hex);
	if (opt_array.has_value())
		return *opt_array;
	else
		return std::nullopt;
}

std::optional<ObjectID> ObjectID::from_raw(std::string_view raw)
{
	ObjectID result{};
	if (raw.size() == result.size())
	{
		std::copy(raw.begin(), raw.end(), result.begin());
		return result;
	}
	else
	{
		return std::nullopt;
	}
}

bool operator==(const ObjectID& id1, const ObjectID& id2)
{
	static_assert(id1.size() == id2.size());    // isn't it obvious?
	return std::memcmp(id1.data(), id2.data(), id1.size()) == 0;
}

bool operator!=(const ObjectID& id1, const ObjectID& id2)
{
	static_assert(id1.size() == id2.size());    // isn't it obvious?
	return std::memcmp(id1.data(), id2.data(), id1.size()) != 0;
}

std::ostream& operator<<(std::ostream& os, const ObjectID& id)
{
	return os << to_hex(id);
}

bool ObjectID::is_hex(std::string_view hex)
{
	return hex.size() == Blake2::size*2 && hex.find_first_not_of("01234567890ABCDEFabcdef") == hex.npos;
}

void from_json(const nlohmann::json& src, ObjectID& dest)
{
	if (auto opt = ObjectID::from_json(src); opt.has_value())
		dest = *opt;
}

void to_json(nlohmann::json& dest, const ObjectID& src)
{
	dest = to_hex(src);
}

} // end of namespace
