/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/26/18.
//

#include "BlobInodeDB.hh"
#include "common/hrb/BlobInode.hh"

#include <cassert>
#include <sstream>

namespace {
	nlohmann::json::json_pointer filename_pointer{"/filename"};
	nlohmann::json::json_pointer mime_pointer{"/mime"};
	nlohmann::json::json_pointer timestamp_pointer{"/timestamp"};
}

namespace hrb {

BlobInodeDB::BlobInodeDB(std::string_view redis_reply) : m_raw{redis_reply}
{
}

std::string BlobInodeDB::create(
	Permission perm, std::string_view filename, std::string_view mime,
	Timestamp timestamp
)
{
	auto json = nlohmann::json::object();
	json.emplace("mime", std::string{mime});
	json.emplace("timestamp", timestamp);

	if (!filename.empty())
		json.emplace("filename", std::string{filename});

	return perm.perm() + json.dump();
}

std::string_view BlobInodeDB::json() const
{
	auto json = m_raw;
	if (!json.empty())
		json.remove_prefix(Permission{}.size());
	return json;
}

std::string BlobInodeDB::filename() const
{
	auto doc = nlohmann::json::parse(BlobInodeDB::json(), nullptr, false);
	return doc.is_discarded() ? "" : doc.value(filename_pointer, "");
}

std::string BlobInodeDB::mime() const
{
	auto doc = nlohmann::json::parse(BlobInodeDB::json(), nullptr, false);
	return doc.is_discarded() ? "" : doc.value(mime_pointer, "");
}

Timestamp BlobInodeDB::timestamp() const
{
	auto doc = nlohmann::json::parse(BlobInodeDB::json(), nullptr, false);
	return doc.is_discarded() ? Timestamp{} : doc.value(timestamp_pointer, Timestamp{});
}

Permission BlobInodeDB::permission() const
{
	return m_raw.empty() ? Permission{} : Permission{m_raw.front()} ;
}

std::string BlobInodeDB::create(Permission perm, const nlohmann::json& json)
{
	return create(
		perm,
		json.value(filename_pointer, ""),
		json.value(mime_pointer, ""),
		json.value(timestamp_pointer, Timestamp{})
	);
}

std::optional<BlobInode> BlobInodeDB::fields() const
{
	auto json = nlohmann::json::parse(BlobInodeDB::json(), nullptr, false);
	if (!json.is_discarded())
		return BlobInode{
			permission(),
			json.value(filename_pointer, ""),
			json.value(mime_pointer, ""),
			json.value(timestamp_pointer, Timestamp{})
		};
	else
		return std::nullopt;
}

std::string BlobInodeDB::create(const BlobInode& fields)
{
	return create(fields.perm, fields.filename, fields.mime, fields.timestamp);
}

} // end of namespace hrb
