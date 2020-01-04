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

	for (auto item : src.at("colls"))
	{
		std::string owner = item.at("owner");
		std::string coll  = item.at("coll");
		item.erase("owner");
		item.erase("coll");

		result.add(owner, coll, std::move(item));
	}

	dest = std::move(result);
}

void to_json(nlohmann::json& dest, const CollectionList& src)
{
	auto colls = nlohmann::json::array();

	for (auto&& en : src.m_entries)
	{
		auto coll = en.meta();
		coll.emplace("owner", std::string{en.owner()});
		coll.emplace("coll", std::string{en.name()});
		colls.push_back(std::move(coll));
	}

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
	return std::equal(
		lhs.m_entries.begin(), lhs.m_entries.end(),
		rhs.m_entries.begin(), rhs.m_entries.end(),
		[](auto&& coll1, auto&& coll2)
		{
			return coll1.name() == coll2.name() && coll1.owner() == coll2.owner() && coll1.meta() == coll2.meta();
		}
	);
}

CollectionList::iterator CollectionList::find(std::string_view owner, std::string_view coll)
{
	return std::find_if(m_entries.begin(), m_entries.end(), [owner, coll](auto&& en)
	{
		return en.owner() == owner && en.name() == coll;
	});
}

CollectionList::const_iterator CollectionList::find(std::string_view owner, std::string_view coll) const
{
	return std::find_if(m_entries.begin(), m_entries.end(), [owner, coll](auto&& en)
	{
		return en.owner() == owner && en.name() == coll;
	});
}

} // end of namespace hrb
