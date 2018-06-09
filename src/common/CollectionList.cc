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

bool operator==(const CollectionList::Entry& lhs, const CollectionList::Entry& rhs)
{
	return lhs.m_name == rhs.m_name && lhs.m_properties == rhs.m_properties;
}

void from_json(const nlohmann::json& src, CollectionList& dest)
{
	CollectionList result;

	for (auto&& item : src["colls"].items())
		result.add(item.key(), item.value());

	dest = std::move(result);
}

void to_json(nlohmann::json& dest, const CollectionList& src)
{
	auto colls = nlohmann::json::object();

	for (auto&& en : src.m_entries)
		colls.emplace(en.name(), en.properties());

	auto result = nlohmann::json::object();
	result.emplace("colls", std::move(colls));

	dest = std::move(result);
}

boost::iterator_range<CollectionList::const_iterator> CollectionList::entries() const
{
	return {m_entries.begin(), m_entries.end()};
}

boost::iterator_range<CollectionList::iterator> CollectionList::entries()
{
	return {m_entries.begin(), m_entries.end()};
}

bool operator==(const CollectionList& lhs, const CollectionList& rhs)
{
	return lhs.m_entries == rhs.m_entries;
}

CollectionList::Entry::Entry(std::string_view name, const ObjectID& cover) :
	m_name{name}, m_properties({{"cover", cover}})
{

}

CollectionList::Entry::Entry(std::string_view name, const nlohmann::json& properties) :
	m_name{name}, m_properties(properties)
{
}

} // end of namespace hrb
