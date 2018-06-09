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
	struct Entry
	{
		std::string     name;
		ObjectID        cover;
	};
	using Entries = std::vector<Entry>;
	using iterator = Entries::const_iterator;

public:
	CollectionList() = default;

	boost::iterator_range<iterator> entries() const;

	void add(std::string_view name, const ObjectID& cover);

	friend void from_json(const nlohmann::json& src, CollectionList& dest);
	friend void to_json(nlohmann::json& dest, const CollectionList& src);

private:
	Entries m_entries;
};

} // end of namespace hrb
