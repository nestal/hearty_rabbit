/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/23/18.
//

#include "Container.hh"

namespace hrb {

const std::string_view Container::redis_prefix{"dir:"};

Container::Container(std::string_view name) : m_name{name}
{
}

rapidjson::Document Container::serialize() const
{
	using namespace rapidjson;
	Document json;
	json.SetObject();
	json.AddMember("name", Value().SetString(m_name.c_str(), m_name.size(), json.GetAllocator()), json.GetAllocator());

	Value elements{kArrayType};
	for (auto&& blob : m_blobs)
	{
		Value e{kObjectType};
		auto bs = to_hex(blob);
		e.AddMember("blob", Value{bs, json.GetAllocator()}, json.GetAllocator());

		elements.PushBack(std::move(e), json.GetAllocator());
	}
	json.AddMember("elements", std::move(elements), json.GetAllocator());

	return json;
}

} // end of namespace hrb
