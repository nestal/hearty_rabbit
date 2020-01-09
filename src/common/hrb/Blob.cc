/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 9/1/2020.
//

#include "Blob.hh"

#include "util/Escape.hh"

#include <cassert>

namespace hrb {

void to_json(nlohmann::json& dest, const BlobElements& src)
{
	auto elements = nlohmann::json::object();

	for (auto&& blob : src)
	{
		nlohmann::json entry_jdoc(blob.info());
		entry_jdoc.emplace("owner", blob.owner());
		entry_jdoc.emplace("collection", blob.collection());
		elements.emplace(to_hex(blob.id()), std::move(entry_jdoc));
	}

	dest = nlohmann::json::object();
	dest.emplace("elements", std::move(elements));
}

void from_json(const nlohmann::json& src, BlobElements& dest)
{
	assert(src.count("elements") > 0);

	for (auto&& kv : src.at("elements").items())
	{
		auto&& key = kv.key();
		auto&& value = kv.value();

		if (auto blob = ObjectID::from_hex(key); blob.has_value())
			dest.emplace_back(
				value.at("owner").get<std::string>(),
				value.at("collection").get<std::string>(),
				*blob,
				value.template get<BlobInode>()
			);
	}
}

} // end of namespace hrb
