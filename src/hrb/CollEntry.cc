/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/26/18.
//

#include "CollEntry.hh"

#include <json.hpp>

#include <cassert>
#include <sstream>

namespace hrb {

CollEntry::CollEntry(std::string_view redis_reply) : m_raw{redis_reply}
{
}

std::string CollEntry::create(
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

std::string_view CollEntry::json() const
{
	auto json = m_raw;
	if (!json.empty())
		json.remove_prefix(Permission{}.size());
	return json;
}

std::string CollEntry::filename() const
{
	auto doc = nlohmann::json::parse(CollEntry::json(), nullptr, false);
	return doc.is_discarded() ? "" : doc.value(nlohmann::json::json_pointer{"/filename"}, "");
}

std::string CollEntry::mime() const
{
	auto doc = nlohmann::json::parse(CollEntry::json(), nullptr, false);
	return doc.is_discarded() ? "" : doc.value(nlohmann::json::json_pointer{"/mime"}, "");
}

Timestamp CollEntry::timestamp() const
{
	auto doc = nlohmann::json::parse(CollEntry::json(), nullptr, false);
	return doc.is_discarded() ? Timestamp{} : doc.value(nlohmann::json::json_pointer{"/timestamp"}, Timestamp{});
}

Permission CollEntry::permission() const
{
	return m_raw.empty() ? Permission{} : Permission{m_raw.front()} ;
}

} // end of namespace hrb
