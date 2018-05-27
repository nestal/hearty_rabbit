/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 5/27/18.
//

#include "Timestamp.hh"

#include <iomanip>
#include <ostream>

namespace hrb {

using namespace std::chrono;
using DoubleMS = duration<double, milliseconds::period>;

void to_json(nlohmann::json& json, const Timestamp& input)
{
	json = input.time_since_epoch().count();
}

void from_json(const nlohmann::json& json, Timestamp& output)
{
	output = Timestamp{Timestamp::duration{json.get<Timestamp::duration::rep>()}};
}

std::ostream& operator<<(std::ostream& os, Timestamp tp)
{
	return os << tp.time_since_epoch().count();
}

Timestamp Timestamp::now()
{
	return time_point_cast<Timestamp::duration>(Timestamp::clock::now());
}

} // end of namespace hrb
