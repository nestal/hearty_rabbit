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

#include "BlobInode.hh"
#include "util/JSON.hh"
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
		BlobInode   entry;
	};

public:
	BlobList() = default;
	BlobList(BlobList&& other) = default;
	BlobList(const BlobList&) = default;
	BlobList& operator=(BlobList&& other) = default;
	BlobList& operator=(const BlobList& other) = default;

	void add(std::string_view owner, std::string_view coll, const ObjectID& blob, const Permission& perm, nlohmann::json&& entry);
	void add(std::string_view owner, std::string_view coll, const ObjectID& blob, const BlobInode& entry);
	void add(const Entry& entry);

	[[nodiscard]] std::vector<Entry> entries() const;
	[[nodiscard]] std::size_t size() const;

	friend void to_json(nlohmann::json& dest, BlobList&& src);
	friend void from_json(const nlohmann::json& src, BlobList& dest);

	[[nodiscard]] const nlohmann::json& json() const {return m_json;}

private:
	nlohmann::json m_json{{"elements", nlohmann::json::object()}};
};

} // end of namespace hrb
