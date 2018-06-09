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

#include <boost/range/iterator_range.hpp>

#include <json.hpp>

#include <string>
#include <vector>

namespace hrb {

class CollectionList
{
public:
	class Entry
	{
	public:
		Entry(std::string_view owner, std::string_view coll, const ObjectID& cover);
		Entry(std::string_view owner, std::string_view coll, const nlohmann::json& properties);

		std::string_view owner() const {return m_owner;}
		std::string_view collection() const {return m_collection;}
		ObjectID cover() const {return m_properties["cover"].template get<ObjectID>();}

		const nlohmann::json& properties() const {return m_properties;}
		nlohmann::json& properties() {return m_properties;}

		friend bool operator==(const Entry& lhs, const Entry& rhs);
		friend bool operator!=(const Entry& lhs, const Entry& rhs) {return !operator==(lhs, rhs);}

	private:
		std::string     m_owner;
		std::string     m_collection;
		nlohmann::json  m_properties;
	};
	using Entries = std::vector<Entry>;
	using iterator = Entries::iterator;
	using const_iterator = Entries::const_iterator;

public:
	CollectionList() = default;

	boost::iterator_range<const_iterator> entries() const;
	boost::iterator_range<iterator> entries();

	iterator find(std::string_view owner, std::string_view coll);
	const_iterator find(std::string_view owner, std::string_view coll) const;

	auto begin() {return m_entries.begin();}
	auto end() {return m_entries.end();}

	auto begin() const {return m_entries.begin();}
	auto end() const {return m_entries.end();}

	template <typename PropertiesOrCover>
	void add(std::string_view owner, std::string_view coll, PropertiesOrCover&& prop)
	{
		m_entries.emplace_back(owner, coll, std::forward<PropertiesOrCover>(prop));
	}

	friend void from_json(const nlohmann::json& src, CollectionList& dest);
	friend void to_json(nlohmann::json& dest, const CollectionList& src);

	friend bool operator==(const CollectionList& lhs, const CollectionList& rhs);
	friend bool operator!=(const CollectionList& lhs, const CollectionList& rhs) {return !operator==(lhs, rhs);}

private:
	Entries m_entries;
};

} // end of namespace hrb
