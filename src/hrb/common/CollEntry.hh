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

#include "hrb/Permission.hh"
#include "hrb/Timestamp.hh"

#include <json.hpp>

namespace hrb {

// There are 3 formats for CollEntry:
// Database format: +{"filename": "image.jpg", "mime": "image/jpeg", "timestamp": 100}
// JSON format:     {"filename": "image.jpg", "mime": "image/jpeg", "timestamp": 100, "perm": "public"}
// In-memory format: CollEntryFields
//
// CollEntryDB represents the Database format.
// CollEntry represent in-memory format.
// nlohmann::json represent JSON format.
//
// Need an explicit and easy API to transform between these 3 formats.
// Make sure the caller of this API understand the persistence and performance implications
// of these transformations.
//
// Only Ownership needs to deal with the database format. (it's the only class to interface with DB)
// Only SessionHandler needs to deal with the JSON format. (it's the only class to send JSON to client)
// Everyone (including Ownership and SessionHandler) may use in-memory format.
//
// Relationship with BlobFile:
// Should BlobFile depends on CollEntry or the other way around?
// Since CollEntry is a simple structure, BlobFile should depend on
// CollEntry, not the other way around.
//
// Dependency graph:
// CollEntryDB (database format) -> CollEntry (in-memory format) -> nlohmann::json (JSON format)
//
// Issues:
// CollEntry::json() should return JSON format, not the substr(1) of the database format.
// to_json(json&, CollEntry) should transform in-memory format to JSON format.
// Database format uses JSON, which is slow to parse and waste space, but it's extensible.

struct CollEntry
{
	Permission  perm;
	std::string filename;
	std::string mime;
	Timestamp   timestamp;
};

} // end of namespace hrb
