/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/6/18.
//

#pragma once

#include "ObjectID.hh"
#include "CollEntry.hh"

#include <boost/range/iterator_range.hpp>

#include <json.hpp>

namespace hrb {

class Collection
{
public:
	using iterator = std::unordered_map<ObjectID, CollEntry>::const_iterator;

public:
	Collection() = default;
	Collection(std::string_view name, std::string_view owner);

	Collection(Collection&&) = default;
	Collection(const Collection&) = default;
	Collection& operator=(Collection&&) = default;
	Collection& operator=(const Collection&) = default;

	auto blobs() const {return boost::iterator_range<iterator>{m_blobs.begin(), m_blobs.end()};}

	friend void from_json(const nlohmann::json& src, Collection& dest);

private:
	std::string     m_name;
	std::string     m_owner;
	ObjectID        m_cover;

	std::unordered_map<ObjectID, CollEntry> m_blobs;
};

} // end of namespace hrb
