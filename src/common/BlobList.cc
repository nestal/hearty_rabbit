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

void BlobList::add(const BlobList::Entry& entry)
{
	return add(entry.owner, entry.coll, entry.blob, entry.entry);
}

std::vector<BlobList::Entry> BlobList::entries() const
{
	std::vector<Entry> result;
	for (auto&& kv : elements().items())
	{
		auto&& key   = kv.key();
		auto&& value = kv.value();

		if (auto blob = ObjectID::from_hex(key); blob.has_value())
			result.push_back(Entry{
				value.at("owner").get<std::string>(),
				value.at("collection").get<std::string>(),
				*blob,
				value.template get<CollEntry>()
			});
	}
	return result;
}

std::size_t BlobList::size() const
{
	return elements().size();
}

void to_json(nlohmann::json& dest, BlobList&& src)
{
	dest = std::move(src.m_json);
}

void from_json(const nlohmann::json& src, BlobList& dest)
{
	dest.m_json = src;
}

const nlohmann::json& BlobList::elements() const
{
	static const auto empty = nlohmann::json::object();
	auto it = m_json.find("elements");
	return it == m_json.end() ? empty : *it;
}

} // end of namespace hrb
