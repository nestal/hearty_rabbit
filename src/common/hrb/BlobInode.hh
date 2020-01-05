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

#include "ObjectID.hh"
#include "Permission.hh"
#include "util/Timestamp.hh"

#include "util/JSON.hh"

#include <unordered_map>

namespace hrb {

// BlobInode represents the data and metadata of a blob in the database. BlobInodes are stored
// in the database for each blob of each user, in the blob_inode:<user> hash table. Even if the same inode
// appears in multiple collections, there is only one copy in the blob inode table.
//
// There are 3 formats for BlobInode:
// Database format: +{"filename": "image.jpg", "mime": "image/jpeg", "timestamp": 100}
// JSON format:     {"filename": "image.jpg", "mime": "image/jpeg", "timestamp": 100, "perm": "public"}
// In-memory format: BlobInode
//
// BlobInodeDB represents the Database format.
// BlobInode represent in-memory format.
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
// Should BlobFile depends on BlobInode or the other way around?
// Since BlobInode is a simple structure, BlobFile should depend on
// BlobInode, not the other way around.
//
// Dependency graph:
// BlobInodeDB (database format) -> BlobInode (in-memory format) -> nlohmann::json (JSON format)
struct BlobInode
{
	/// Permission data that indicates who can access this inode.
	Permission  perm;

	/// When stored in blob_inode:<owner> in database, it indicates the original filename when the
	/// blob is uploaded. When BlobInode is used to store an entry in a collection (e.g. when returned
	/// by Ownership::get_collection()), this field indicates the filename in that collection, and not
	/// the original filename.
	std::string filename;

	/// Mime type of the blob. Should be the same for all renditions.
	std::string mime;

	/// Creation time of the blob. For image it should be determined by EXIF tags. It that is not available
	/// then the upload time will be used as a last restore.
	Timestamp   timestamp;
};

void to_json(nlohmann::json& dest, const BlobInode& src);
void from_json(const nlohmann::json& src, BlobInode& dest);

} // end of namespace hrb
