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
		Entry(std::string_view name, const ObjectID& cover);
		Entry(std::string_view name, const nlohmann::json& properties);

		std::string_view name() const {return m_name;}
		ObjectID cover() const {return m_properties["cover"].template get<ObjectID>();}

		const nlohmann::json& properties() const {return m_properties;}

		friend bool operator==(const Entry& lhs, const Entry& rhs);
		friend bool operator!=(const Entry& lhs, const Entry& rhs) {return !operator==(lhs, rhs);}

	private:
		std::string     m_name;
		nlohmann::json  m_properties;
	};
	using Entries = std::vector<Entry>;
	using iterator = Entries::const_iterator;

public:
	CollectionList() = default;

	boost::iterator_range<iterator> entries() const;

	template <typename PropertiesOrCover>
	void add(std::string_view name, PropertiesOrCover&& prop)
	{
		m_entries.emplace_back(name, std::forward<PropertiesOrCover>(prop));
	}

	friend void from_json(const nlohmann::json& src, CollectionList& dest);
	friend void to_json(nlohmann::json& dest, const CollectionList& src);

	friend bool operator==(const CollectionList& lhs, const CollectionList& rhs);
	friend bool operator!=(const CollectionList& lhs, const CollectionList& rhs) {return !operator==(lhs, rhs);}

private:
	Entries m_entries;
};

} // end of namespace hrb