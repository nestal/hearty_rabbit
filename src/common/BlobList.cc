/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/10/18.
//

#include "BlobList.hh"

#include "ObjectID.hh"
#include "Permission.hh"
#include "CollEntry.hh"
#include "Escape.hh"

namespace hrb {

void BlobList::add(std::string_view owner, std::string_view coll, const ObjectID& blob, const CollEntry& entry)
{
	nlohmann::json entry_jdoc(entry);
	entry_jdoc.emplace("owner", owner);
	entry_jdoc.emplace("collection", coll);
	m_json["elements"].emplace(to_hex(blob), std::move(entry_jdoc));
}

void BlobList::add(std::string_view owner, std::string_view coll, const ObjectID& blob, const Permission& perm, nlohmann::json&& entry)
{
	auto entry_jdoc(std::move(entry));
	entry_jdoc.emplace("perm", perm.description());
	entry_jdoc.emplace("owner", owner);
	entry_jdoc.emplace("collection", coll);
	m_json["elements"].emplace(to_hex(blob), std::move(entry_jdoc));
}

} // end of namespace hrb
