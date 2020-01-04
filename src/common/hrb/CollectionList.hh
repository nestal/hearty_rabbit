/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/9/18.
//

#pragma once

#include "ObjectID.hh"
#include "Collection.hh"

#include <boost/range/iterator_range.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace hrb {

class CollectionList
{
public:
	using Entries = std::vector<Collection>;
	using iterator = Entries::iterator;
	using const_iterator = Entries::const_iterator;

public:
	CollectionList() = default;

	[[nodiscard]] boost::iterator_range<const_iterator> entries() const;
	[[nodiscard]] boost::iterator_range<iterator> entries();

	[[nodiscard]] iterator find(std::string_view owner, std::string_view coll);
	[[nodiscard]] const_iterator find(std::string_view owner, std::string_view coll) const;

	[[nodiscard]] auto begin() {return m_entries.begin();}
	[[nodiscard]] auto end() {return m_entries.end();}

	[[nodiscard]] auto begin() const {return m_entries.begin();}
	[[nodiscard]] auto end() const {return m_entries.end();}
	[[nodiscard]] auto size() const {return m_entries.size();}

	template <typename PropertiesOrCover>
	void add(std::string_view owner, std::string_view coll, PropertiesOrCover&& prop)
	{
		m_entries.emplace_back(coll, owner, std::forward<PropertiesOrCover>(prop));
	}

	friend void from_json(const nlohmann::json& src, CollectionList& dest);
	friend void to_json(nlohmann::json& dest, const CollectionList& src);

	friend bool operator==(const CollectionList& lhs, const CollectionList& rhs);
	friend bool operator!=(const CollectionList& lhs, const CollectionList& rhs) {return !operator==(lhs, rhs);}

private:
	Entries m_entries;
};

} // end of namespace hrb
