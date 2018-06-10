/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/10/18.
//

#pragma once

#include "CollEntry.hh"
#include "JSON.hh"
#include "ObjectID.hh"

#include <boost/range/adaptors.hpp>

namespace hrb {

class Permission;

class BlobList
{
public:
	struct Entry
	{
		std::string owner;
		std::string coll;
		ObjectID    blob;
		CollEntry   entry;
	};

public:
	BlobList() = default;

	void add(std::string_view owner, std::string_view coll, const ObjectID& blob, const Permission& perm, nlohmann::json&& entry);
	void add(std::string_view owner, std::string_view coll, const ObjectID& blob, const CollEntry& entry);
	void add(const Entry& entry);

	auto entries() const
	{
		using namespace boost::adaptors;
		return m_json | transformed([](auto&& kv)
		{
			auto&& key   = kv.first;
			auto&& value = kv.second;
			return Entry{value.at("owner"), value.at("coll"), ObjectID::from_hex(key), value.template get<CollEntry>()};
		});
	}

private:
	nlohmann::json m_json;
};

} // end of namespace hrb
