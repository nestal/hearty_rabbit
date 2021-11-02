/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 12 Feb 2018.
//

#pragma once

#include "crypto/Blake2.hh"

#include <boost/functional/hash.hpp>

#include <nlohmann/json.hpp>

#include <array>
#include <iosfwd>
#include <optional>
#include <string_view>
#include <type_traits>

namespace hrb {

struct ObjectID : std::array<unsigned char, Blake2::size>
{
	using array::array;
	explicit ObjectID(const std::array<unsigned char, Blake2::size>& array);

	static std::optional<ObjectID> from_hex(std::string_view hex);
	static std::optional<ObjectID> from_raw(std::string_view raw);
	static std::optional<ObjectID> from_json(const nlohmann::json& json);
	static bool is_hex(std::string_view hex);

	static ObjectID randomize();
};
static_assert(std::is_standard_layout<ObjectID>::value);

void from_json(const nlohmann::json& src, ObjectID& dest);
void to_json(nlohmann::json& dest, const ObjectID& src);

} // end of namespace

// inject hash<> to std namespace for unordered_map
namespace std
{
    template<> struct hash<hrb::ObjectID>
    {
        typedef hrb::ObjectID argument_type;
        typedef std::size_t result_type;
        result_type operator()(const argument_type& s) const noexcept
		{
			struct FiveU32
			{
				std::uint32_t i[5];
			};

			FiveU32 t{};
//			static_assert(sizeof(t) == s.size());
			std::memcpy(&t, &s[0], s.size());

			std::size_t seed = 0;
			for (auto&& i : t.i)
				boost::hash_combine(seed, i);
			return seed;
		}
   };
}
