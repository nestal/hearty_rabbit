/*
	Copyright © 2019 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/16/19.
//

#pragma once

#include <string>

namespace hrb {
class ObjectID;
}

namespace hrb::key {

/// blob_refs is a redis set that contains all collections a blob belongs to for a specific user.
std::string blob_refs(std::string_view user, const ObjectID& blob);

/// blob_owners is a redis set that contains the names of all users that has a specific blob.
std::string blob_owners(const ObjectID& blob);

/// collection is a redis set that contains a list of raw blob IDs
std::string collection(std::string_view user, std::string_view coll);

// collection_list is a redis hash that contains a list of collections owned by a specific user.
std::string collection_list(std::string_view user);

// public_blobs is a redis list that contains a list of raw blob IDs are public.
std::string public_blobs();

// blob_meta is a redis hash that contains meta-data about a specific blob of a specific user
std::string blob_meta(std::string user, const ObjectID& blob);

} // end of namespace
