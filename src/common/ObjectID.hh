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

#include "../server/crypto/Blake2.hh"

#include <array>
#include <iosfwd>
#include <optional>
#include <string_view>
#include <type_traits>

namespace hrb {

using ObjectID = std::array<unsigned char, Blake2::size>;
static_assert(std::is_standard_layout<ObjectID>::value);

std::optional<ObjectID> hex_to_object_id(std::string_view hex);
bool is_valid_blob_id(std::string_view hex);

std::optional<ObjectID> raw_to_object_id(std::string_view raw);

} // end of namespace
