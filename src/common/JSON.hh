/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/10/18.
//

#pragma once

#include <string_view>
#include <json.hpp>

// partial specialization (full specialization works too)
namespace nlohmann {

template <>
struct adl_serializer<std::string_view>
{
	static void to_json(json& j, std::string_view sv)
	{
		j = std::string{sv};
	}
};

} // end of namespace