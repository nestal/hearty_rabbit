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
#include "util/JsonHelper.hh"

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/pointer.h>
#include <rapidjson/writer.h>

#include <cassert>
#include <sstream>

namespace hrb {

CollEntry::CollEntry(std::string_view redis_reply) : m_raw{redis_reply}
{
}

std::string CollEntry::create(Permission perm, std::string_view filename, std::string_view mime)
{
	rapidjson::Document json;
	json.SetObject();
	json.AddMember("mime", json::string_ref(mime), json.GetAllocator());

	if (!filename.empty())
		json.AddMember("filename", json::string_ref(filename), json.GetAllocator());

	std::ostringstream ss;
	ss << perm.perm();

	rapidjson::OStreamWrapper osw{ss};
	rapidjson::Writer<rapidjson::OStreamWrapper> writer{osw};
	json.Accept(writer);
	return ss.str();
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

Permission CollEntry::permission() const
{
	return m_raw.empty() ? Permission{} : Permission{m_raw.front()} ;
}

} // end of namespace hrb
