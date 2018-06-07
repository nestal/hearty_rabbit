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

void from_json(const nlohmann::json& src, std::unordered_map<ObjectID, CollEntry>& dest)
{
	std::unordered_map<ObjectID, CollEntry> result;

	for (auto&& item : src["elements"].items())
	{
		if (auto blob = ObjectID::from_hex(item.key()); blob.has_value())
			result.emplace(*blob, item.value().template get<CollEntry>());
	}

	// commit changes
	dest = std::move(result);
}

} // end of namespace
