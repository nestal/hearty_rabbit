/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/5/18.
//

#include "BlobInode.hh"

namespace hrb {

void from_json(const nlohmann::json& src, BlobInode& dest)
{
	dest.timestamp = src.at("timestamp");
	dest.filename  = src.at("filename");
	dest.mime      = src.at("mime");
	dest.perm      = Permission::from_description(src.at("perm").get<std::string>());
}

void to_json(nlohmann::json& dest, const BlobInode& src)
{
	auto result = nlohmann::json::object();
	result.emplace("timestamp", src.timestamp);
	result.emplace("filename",  src.filename);
	result.emplace("mime", src.mime);
	result.emplace("perm", src.perm.description());

	dest = std::move(result);
}

} // end of namespace
