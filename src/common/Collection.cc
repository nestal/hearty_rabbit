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
#include "Escape.hh"

namespace hrb {

Collection::Collection(std::string_view name, std::string_view owner) :
	m_name{name}, m_owner{owner}
{
}

void Collection::add_blob(const ObjectID& id, CollEntry&& entry)
{
	m_blobs.emplace(id, std::move(entry));
}

void Collection::add_blob(const ObjectID& id, const CollEntry& entry)
{
	m_blobs.emplace(id, entry);
}

void from_json(const nlohmann::json& src, Collection& dest)
{
	Collection result;

	result.m_name   = src["collection"];
	result.m_owner  = src["owner"];

	for (auto&& item : src["elements"].items())
	{
		if (auto blob = ObjectID::from_hex(item.key()); blob.has_value())
			result.m_blobs.emplace(*blob, item.value().template get<CollEntry>());
	}

	// commit changes
	dest = std::move(result);
}

void to_json(nlohmann::json& dest, const Collection& src)
{
	auto result = nlohmann::json::object();
	result["collection"] = src.m_name;
	result["owner"]      = src.m_owner;

	auto blobs = nlohmann::json::object();
	for (auto&& [id, entry] : src.m_blobs)
		blobs.emplace(to_hex(id), entry);

	result.emplace("elements", std::move(blobs));

	// commit changes
	dest = std::move(result);
}

} // end of namespace hrb
