/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/6/18.
//

#include "Collection.hh"

namespace hrb {

Collection::Collection(std::string_view name, std::string_view owner) :
	m_name{name}, m_owner{owner}
{
}

void from_json(const nlohmann::json& src, Collection& dest)
{
	Collection result;

	result.m_name   = src["collection"];
	result.m_owner  = src["owner"];

	for (auto&& item : src["elements"].items())
	{
		if (auto blob = hrb::hex_to_object_id(item.key()); blob.has_value())
			result.m_blobs.emplace(*blob, item.value().template get<CollEntry>());
	}

	// commit changes
	dest = std::move(result);
}

} // end of namespace hrb
