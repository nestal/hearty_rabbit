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
	json["name"] = Value().SetString(m_name.c_str(), m_name.size(), json.GetAllocator());

	Value elements(kArrayType);
	for (auto&& blob : m_blobs)
	{
		auto bs = to_hex(blob);
		elements.PushBack(Value{bs, json.GetAllocator()}, json.GetAllocator());
	}
	json["elements"] = std::move(elements);

	return json;
}

} // end of namespace hrb
