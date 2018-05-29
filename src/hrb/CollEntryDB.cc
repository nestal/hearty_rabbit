/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/26/18.
//

#include "CollEntryDB.hh"

#include <cassert>
#include <sstream>

namespace {
	nlohmann::json::json_pointer filename_pointer{"/filename"};
	nlohmann::json::json_pointer mime_pointer{"/mime"};
	nlohmann::json::json_pointer timestamp_pointer{"/timestamp"};
}

namespace hrb {

CollEntryDB::CollEntryDB(std::string_view redis_reply) : m_raw{redis_reply}
{
}

std::string CollEntryDB::create(
	Permission perm, std::string_view filename, std::string_view mime,
	Timestamp timestamp
)
{
	auto json = nlohmann::json::object();
	json.emplace("mime", std::string{mime});
	json.emplace("timestamp", timestamp);

	if (!filename.empty())
		json.emplace("filename", std::string{filename});

	return perm.perm() + json.dump();
}

std::string_view CollEntryDB::json() const
{
	auto json = m_raw;
	if (!json.empty())
		json.remove_prefix(Permission{}.size());
	return json;
}

std::string CollEntryDB::filename() const
{
	auto doc = nlohmann::json::parse(CollEntryDB::json(), nullptr, false);
	return doc.is_discarded() ? "" : doc.value(filename_pointer, "");
}

std::string CollEntryDB::mime() const
{
	auto doc = nlohmann::json::parse(CollEntryDB::json(), nullptr, false);
	return doc.is_discarded() ? "" : doc.value(mime_pointer, "");
}

Timestamp CollEntryDB::timestamp() const
{
	auto doc = nlohmann::json::parse(CollEntryDB::json(), nullptr, false);
	return doc.is_discarded() ? Timestamp{} : doc.value(timestamp_pointer, Timestamp{});
}

Permission CollEntryDB::permission() const
{
	return m_raw.empty() ? Permission{} : Permission{m_raw.front()} ;
}

std::string CollEntryDB::create(Permission perm, const nlohmann::json& json)
{
	return create(
		perm,
		json.value(filename_pointer, ""),
		json.value(mime_pointer, ""),
		json.value(timestamp_pointer, Timestamp{})
	);
}

CollEntry CollEntryDB::fields() const
{
	auto json = nlohmann::json::parse(CollEntryDB::json(), nullptr, false);
	return {
		permission(),
		json.value(filename_pointer, ""),
		json.value(mime_pointer, ""),
		json.value(timestamp_pointer, Timestamp{})
	};
}

std::string CollEntryDB::create(const CollEntry& fields)
{
	return create(fields.perm, fields.filename, fields.mime, fields.timestamp);
}

} // end of namespace hrb
