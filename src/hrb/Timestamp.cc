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
	json = duration_cast<DoubleMS>(input.time_since_epoch()).count();
}

void from_json(const nlohmann::json& json, Timestamp& output)
{
	output = Timestamp{duration_cast<Timestamp::duration>(DoubleMS{json.get<double>()})};
}

std::ostream& operator<<(std::ostream& os, Timestamp tp)
{
	return os << tp.time_since_epoch().count();
}

} // end of namespace hrb
