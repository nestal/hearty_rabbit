/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/9/18.
//

#include "CollectionList.hh"

namespace hrb {

void from_json(const nlohmann::json& src, CollectionList& dest)
{
	CollectionList result;

	for (auto&& item : src["colls"].items())
		result.add(item.key(), item.value()["cover"].template get<ObjectID>());

	dest = std::move(result);
}

void to_json(nlohmann::json& dest, const CollectionList& src)
{
	auto colls = nlohmann::json::object();

	for (auto&& en : src.m_entries)
		colls.emplace(en.name, nlohmann::json::object({{"cover", en.cover}}));

	auto result = nlohmann::json::object();
	result.emplace("colls", std::move(colls));

	dest = std::move(result);
}

boost::iterator_range<CollectionList::iterator> CollectionList::entries() const
{
	return {m_entries.begin(), m_entries.end()};
}

void CollectionList::add(std::string_view name, const ObjectID& cover)
{
	m_entries.push_back(CollectionList::Entry{std::string{name}, cover});
}

} // end of namespace hrb
