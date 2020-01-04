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
#include "BlobInode.hh"
#include "common/util/Escape.hh"

namespace hrb {

void BlobList::add(std::string_view owner, std::string_view coll, const ObjectID& blob, const BlobInode& entry)
{
	assert(m_json.count("elements") > 0);

	nlohmann::json entry_jdoc(entry);
	entry_jdoc.emplace("owner", owner);
	entry_jdoc.emplace("collection", coll);
	m_json.at("elements").emplace(to_hex(blob), std::move(entry_jdoc));
}

void BlobList::add(std::string_view owner, std::string_view coll, const ObjectID& blob, const Permission& perm, nlohmann::json&& entry)
{
	assert(m_json.count("elements") > 0);

	auto entry_jdoc(std::move(entry));
	entry_jdoc.emplace("perm", perm.description());
	entry_jdoc.emplace("owner", owner);
	entry_jdoc.emplace("collection", coll);
	m_json.at("elements").emplace(to_hex(blob), std::move(entry_jdoc));
}

void BlobList::add(const BlobList::Entry& entry)
{
	return add(entry.owner, entry.coll, entry.blob, entry.entry);
}

std::vector<BlobList::Entry> BlobList::entries() const
{
	assert(m_json.count("elements") > 0);

	std::vector<Entry> result;
	for (auto&& kv : m_json.at("elements").items())
	{
		auto&& key   = kv.key();
		auto&& value = kv.value();

		if (auto blob = ObjectID::from_hex(key); blob.has_value())
			result.push_back(Entry{
				value.at("owner").get<std::string>(),
				value.at("collection").get<std::string>(),
				*blob,
				value.template get<BlobInode>()
			});
	}
	return result;
}

std::size_t BlobList::size() const
{
	return m_json.at("elements").size();
}

void to_json(nlohmann::json& dest, BlobList&& src)
{
	assert(src.m_json.count("elements") > 0);

	dest = std::move(src.m_json);
	src.m_json = nlohmann::json::object({{"elements", {}}});

	if (!dest.is_object())
		dest = nlohmann::json::object();

	assert(src.m_json.count("elements") > 0);
}

void from_json(const nlohmann::json& src, BlobList& dest)
{
	assert(dest.m_json.count("elements") > 0);

	dest.m_json = src;
	if (!dest.m_json.is_object())
		dest.m_json = nlohmann::json::object();
	if (dest.m_json.find("elements") == dest.m_json.end())
		dest.m_json.emplace("elements", nlohmann::json::object());

	assert(dest.m_json.count("elements") > 0);
}

} // end of namespace hrb
