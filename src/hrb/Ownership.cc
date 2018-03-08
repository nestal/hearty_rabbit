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
#include "BlobDatabase.hh"

#include <sstream>

namespace hrb {

const std::string_view Ownership::redis_prefix{"ownership:"};

namespace detail {
const std::string_view Prefix<ObjectID>::value{"blob:"};
const std::string_view Prefix<ContainerName>::value{"dir:"};
}

Ownership::Ownership(std::string_view name) : m_name{name}
{
}

std::string Ownership::serialize(const BlobDatabase& db) const
{
	bool first = true;
	std::ostringstream json;
	json << R"({"name":")" << m_name << R"(", "elements":{)";
	for (auto&& blob : m_blobs)
	{
		auto meta = db.load_meta_json(blob);
		if (!meta.empty())
		{
			if (first)
				first = false;
			else
				json << ",\n";

			json << '\"' << blob << R"(": )" << meta;
		}
	}
	json << "}}";

	return json.str();
}

} // end of namespace hrb
