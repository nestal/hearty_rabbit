/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#include "Container.hh"

#include <rapidjson/document.h>
#include <rapidjson/pointer.h>

namespace hrb {

const std::string_view Container::redis_prefix = "dir:";

Container::Container(std::string_view user, std::string_view path) :
	m_user{user},
	m_path{path}
{
}

std::string Container::find(const std::string& filename) const
{
	auto it = m_jsons.find(filename);
	return it != m_jsons.end() ? it->second : std::string{};
}

std::optional<Entry> Container::find_entry(const std::string& filename) const
{
	auto json = find(filename);
	if (!json.empty())
		return Entry{filename, json};
	else
		return std::nullopt;
}

std::string Container::serialize(std::string&& user, std::string&& path, redis::Reply& reply)
{
	std::ostringstream ss;
	ss  << R"__({"name":")__"      << user
		<< R"__(", "path":")__"    << path
		<< R"__(", "elements":)__" << "[";

	bool first = true;
	reply.foreach_kv_pair([&ss, &first](auto&&, auto&& json)
	{
		if (first)
			first = false;
		else
			ss << ",\n";

		ss << json.as_string();
	});
	ss << "]}";
	return ss.str();
}

std::string Entry::JSON(const ObjectID& blob, std::string_view mime, std::string_view filename)
{
	// too simple to bother the json library
	std::ostringstream ss;
	ss  << "{"
		<< R"("blob_id": ")"    << to_hex(blob) << "\","
		<< R"("mime": ")"       << mime         << "\","
		<< R"("filename": ")"   << filename     << "\""
		<< "}";
	return ss.str();
}

std::string Entry::JSON() const
{
	return JSON(m_blob, m_mime, m_filename);
}

Entry::Entry(std::string_view filename, std::string_view json) : m_filename{filename}
{
	rapidjson::Document doc;
	doc.Parse(json.data(), json.size());

	m_blob = hex_to_object_id(GetValueByPointerWithDefault(doc, "/blob_id", "").GetString());
	m_mime = GetValueByPointerWithDefault(doc, "/mime", "").GetString();
}

} // end of namespace hrb
