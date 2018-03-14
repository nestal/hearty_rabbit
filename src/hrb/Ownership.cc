/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/23/18.
//

#include "Ownership.hh"
#include "Ownership.ipp"
#include "BlobDatabase.hh"

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/pointer.h>
#include <rapidjson/writer.h>

#include <sstream>

namespace hrb {

Ownership::Ownership(std::string_view name) : m_user{name}
{
}

const std::string_view Ownership::BlobBackLink::m_prefix{"blob-backlink:"};

Ownership::BlobBackLink::BlobBackLink(std::string_view user, const ObjectID& blob) :
	m_user{user}, m_blob{blob}
{
}

void Ownership::BlobBackLink::watch(redis::Connection& db) const
{
	db.command(
		"WATCH %b%b:%b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size()
	);
}

void Ownership::BlobBackLink::link(redis::Connection& db, std::string_view coll) const
{
	db.command(
		"SADD %b%b:%b %b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size(),
		coll.data(), coll.size()
	);
}

void Ownership::BlobBackLink::unlink(redis::Connection& db, std::string_view coll) const
{
	db.command(
		"SREM %b%b:%b %b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size(),
		coll.data(), coll.size()
	);
}

const std::string_view Ownership::Collection::m_prefix = "dir:";

Ownership::Collection::Collection(std::string_view user, std::string_view path) :
	m_user{user},
	m_path{path}
{
}

void Ownership::Collection::watch(redis::Connection& db)
{
	db.command("WATCH %b%b:%b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(), m_path.data(), m_path.size()
	);
}

void Ownership::Collection::link(redis::Connection& db, const ObjectID& id, const CollEntry& entry)
{
	db.command("HSET %b%b:%b %b %b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		id.data(), id.size(),
		entry.data(), entry.size()
	);
}

void Ownership::Collection::unlink(redis::Connection& db, const ObjectID& id)
{
	db.command("HDEL %b%b:%b %b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		id.data(), id.size()
	);
}

CollEntry::CollEntry(std::string_view redis_reply) : m_raw{redis_reply}
{
}

std::string CollEntry::create(std::string_view perm, std::string_view filename, std::string_view mime)
{
	rapidjson::Document json;
	json.SetObject();
	json.AddMember("mime", rapidjson::StringRef(mime.data(), mime.size()), json.GetAllocator());

	if (!filename.empty())
		json.AddMember("filename", rapidjson::StringRef(filename.data(), filename.size()), json.GetAllocator());

	std::ostringstream ss;
	ss << perm;

	rapidjson::OStreamWrapper osw{ss};
	rapidjson::Writer<rapidjson::OStreamWrapper> writer{osw};
	json.Accept(writer);
	return ss.str();
}

std::string_view CollEntry::json() const
{
	auto json = m_raw;
	json.remove_prefix(1);
	return json;
}

std::string CollEntry::filename() const
{
	auto json = CollEntry::json();

	rapidjson::Document doc;
	doc.Parse(json.data(), json.size());
	if (doc.HasParseError())
		return {};

	auto&& val = GetValueByPointerWithDefault(doc, "/filename", "");
	return std::string{val.GetString(), val.GetStringLength()};
}

std::string CollEntry::mime() const
{
	auto json = CollEntry::json();

	rapidjson::Document doc;
	doc.Parse(json.data(), json.size());
	if (doc.HasParseError())
		return {};

	auto&& val = GetValueByPointerWithDefault(doc, "/mime", "");
	return std::string{val.GetString(), val.GetStringLength()};
}

bool CollEntry::allow(std::string_view user) const
{
	return Permission{m_raw.substr(0, 1)}.allow(user);
}

} // end of namespace hrb
