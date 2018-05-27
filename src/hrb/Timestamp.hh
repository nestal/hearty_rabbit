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

using TimePointBase = std::chrono::time_point<
    std::chrono::system_clock,
	std::chrono::milliseconds
>;

/// \brief  The unit of timestamp stored in database.
/// It is currently the number of miniseconds since the unix epoch
struct Timestamp : TimePointBase
{
	using time_point::time_point;
	Timestamp(TimePointBase tp) : Timestamp{tp.time_since_epoch()} {}
};

void to_json(nlohmann::json& json, const Timestamp& input);
void from_json(const nlohmann::json& json, Timestamp& output);

std::ostream& operator<<(std::ostream& os, Timestamp tp);

} // end of namespace hrb
