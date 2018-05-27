/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 5/27/18.
//

#pragma once

#include <json.hpp>
#include <chrono>
#include <iosfwd>

namespace hrb {

struct Timestamp : std::chrono::system_clock::time_point
{
	using time_point::time_point;
	Timestamp(std::chrono::system_clock::time_point tp) : Timestamp{tp.time_since_epoch()} {}
};

void to_json(nlohmann::json& json, const Timestamp& input);
void from_json(const nlohmann::json& json, Timestamp& output);

std::ostream& operator<<(std::ostream& os, Timestamp tp);

} // end of namespace hrb
