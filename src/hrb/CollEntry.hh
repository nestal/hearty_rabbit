/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/26/18.
//

#pragma once

#include "Permission.hh"
#include "Timestamp.hh"

#include <json.hpp>
#include <string>

namespace hrb {

// There are 3 formats for CollEntry:
// Database format: +{"filename": "image.jpg", "mime": "image/jpeg", "timestamp": 100}
// JSON format:     {"filename": "image.jpg", "mime": "image/jpeg", "timestamp": 100, "perm": "public"}
// In-memory format: CollEntryFields
//
// CollEntry represents the Database format.
// CollEntryFields represent in-memory format.
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
// Should BlobFile depends on CollEntryFields or the other way around?
// Since CollEntryFields is a simple structure, BlobFile should depend on
// CollEntryFields, not the other way around.
//
// Dependency graph:
// CollEntry (database format) -> CollEntryFields (in-memory format) -> nlohmann::json (JSON format)
//
// Issues:
// CollEntry::json() should return JSON format, not the substr(1) of the database format.
// to_json(json&, CollEntryFields) should transform in-memory format to JSON format.
// Database format uses JSON, which is slow to parse and waste space, but it's extensible.

struct CollEntryFields
{
	Permission  perm;
	std::string filename;
	std::string mime;
	Timestamp   timestamp;
};

// along with from_json(CollEntryFields) and to_json(CollEntryFields), CollEntryFields should
// be moved to a separate header

class CollEntry
{
public:
	CollEntry() = default;
	explicit CollEntry(std::string_view redis_reply);

	static std::string create(
		Permission perm, std::string_view filename, std::string_view mime,
		Timestamp timestamp
	);
	static std::string create(Permission perm, const nlohmann::json& json);
	static std::string create(const CollEntryFields& fields);

	std::string filename() const;
	std::string mime() 	const;
	Timestamp timestamp() const;
	CollEntryFields fields() const;

	std::string_view json() const;
	Permission permission() const;

	std::string_view raw() const {return m_raw;}
	auto data() const {return m_raw.data();}
	auto size() const {return m_raw.size();}

private:
	std::string_view	m_raw{" {}"};
};

} // end of namespace hrb
