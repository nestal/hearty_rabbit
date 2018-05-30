/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/30/18.
//

#include "BlobRef.hh"

namespace hrb {

void to_json(nlohmann::json& dest, const BlobRef& src)
{
	auto jdoc = nlohmann::json::parse(src.entry.json(), nullptr, false);
	if (!jdoc.is_discarded())
	{
		jdoc.emplace("perm", std::string{src.entry.permission().description()});
		jdoc.emplace("owner", src.user);
		jdoc.emplace("collection", src.coll);
		dest = std::move(jdoc);
	}
}

} // end of namespace
