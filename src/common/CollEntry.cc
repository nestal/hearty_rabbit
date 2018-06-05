/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/5/18.
//

#include "CollEntry.hh"

namespace hrb {

void from_json(const nlohmann::json& src, CollEntry& dest)
{
	dest.timestamp = src["timestamp"];
	dest.filename  = src["filename"];
	dest.mime      = src["mime"];
	dest.perm      = Permission::from_description(src["perm"].get<std::string>());
}

} // end of namespace
