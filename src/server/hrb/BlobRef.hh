/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/30/18.
//

#pragma once

#include "CollEntryDB.hh"
#include "common/ObjectID.hh"

#include <string>
#include <json.hpp>

namespace hrb {

/// \brief  A structure to store information about a blob in the database
/// The memory pointed by the CollEntryDB will be freed after the callback
/// function returns.
struct BlobRef
{
	std::string user;
	std::string coll;
	ObjectID    blob;
	CollEntryDB entry;
};

void to_json(nlohmann::json& dest, const BlobRef& src);

} // end of namespace
