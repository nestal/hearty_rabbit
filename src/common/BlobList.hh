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

#include "JSON.hh"

namespace hrb {

class CollEntry;
class ObjectID;
class Permission;

class BlobList
{
public:
	struct Entry
	{

	};

public:
	BlobList() = default;

	void add(std::string_view owner, std::string_view coll, const ObjectID& blob, const Permission& perm, nlohmann::json&& entry);
	void add(std::string_view owner, std::string_view coll, const ObjectID& blob, const CollEntry& entry);

private:
	nlohmann::json m_json;
};

} // end of namespace hrb
