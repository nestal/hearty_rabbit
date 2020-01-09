/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/6/18.
//

#include "Collection.hh"
#include "Blob.hh"

#include "util/Escape.hh"
#include "util/MMap.hh"
#include "util/Magic.hh"
#include "image/Image.hh"

#include <algorithm>

namespace hrb {

Collection::Collection(std::string_view name, std::string_view owner, nlohmann::json&& meta) :
	m_name{name}, m_owner{owner}, m_meta(std::move(meta))
{
	assert(m_meta.is_object() || m_meta.is_null());
}

Collection::Collection(std::string_view name, std::string_view owner, const ObjectID& cover) :
	m_name{name}, m_owner{owner}, m_meta({{"cover", to_hex(cover)}})
{
	assert(m_meta.is_object());
}

Collection::Collection(const std::filesystem::path& path) :
	m_name{path.filename() == "." ? path.parent_path().filename() : path.filename()}
{
	for (auto&& file : std::filesystem::directory_iterator{path})
	{
		std::error_code ec;
		auto mmap = MMap::open(file.path(), ec);
		if (!ec)
		{
			Blake2 hash;
			hash.update(mmap.data(), mmap.size());
			auto blob = hash.finalize();

			ImageMeta meta{mmap.buffer()};
//			std::cout << to_hex(blob) << " " << file.path().filename() << " " << meta.mime() << " " <<
//				meta.original_timestamp().value_or(Timestamp{}).http_format() << std::endl;

			add_blob(
				blob,
				BlobInode{
					{}, file.path().filename(), std::string{meta.mime()},
					meta.original_timestamp().value_or(Timestamp{})
				}
			);
		}
	}
}

void Collection::add_blob(const ObjectID& id, BlobInode&& entry)
{
	m_blobs.emplace(id, std::move(entry));
}

void Collection::add_blob(const ObjectID& id, const BlobInode& entry)
{
	m_blobs.emplace(id, entry);
}

void from_json(const nlohmann::json& src, Collection& dest)
{
	Collection result;

	result.m_name   = src.at("collection");
	result.m_owner  = src.at("owner");
	result.m_meta   = src.at("meta");

	for (auto&& item : src.at("elements").items())
	{
		if (auto blob = ObjectID::from_hex(item.key()); blob.has_value())
			result.m_blobs.emplace(*blob, item.value().template get<BlobInode>());
	}

	// commit changes
	dest = std::move(result);
}

void to_json(nlohmann::json& dest, const Collection& src)
{
	auto result = nlohmann::json::object();
	result.emplace("collection",  src.m_name);
	result.emplace("owner",       src.m_owner);
	result.emplace("meta",        src.m_meta);

	auto blobs = nlohmann::json::object();
	for (auto&& [id, entry] : src.m_blobs)
		blobs.emplace(to_hex(id), entry);

	result.emplace("elements", std::move(blobs));

	// commit changes
	dest = std::move(result);
}

Collection::iterator Collection::find(const ObjectID& id) const
{
	return m_blobs.find(id);
}

std::optional<ObjectID> Collection::cover() const
{
	if (auto it = m_meta.find("cover"); it != m_meta.end())
		return ObjectID::from_json(*it);
	else
		return std::nullopt;
}

void Collection::update_timestamp(const ObjectID& id, Timestamp value)
{
	if (auto it = m_blobs.find(id); it != m_blobs.end())
		it->second.timestamp = value;
}

void Collection::remove_blob(const ObjectID& id)
{
	m_blobs.erase(id);
}

std::optional<Blob> Collection::get_blob(const ObjectID& id) const
{
	if (auto it = m_blobs.find(id); it != m_blobs.end())
		return Blob{m_owner, m_name, id, it->second};
	else
		return std::nullopt;
}

bool Collection::operator==(const Collection& rhs) const
{
	return m_name == rhs.m_name && m_owner == rhs.m_owner && m_meta == rhs.m_meta &&
		m_blobs.size() == rhs.m_blobs.size() &&
		std::equal(m_blobs.begin(), m_blobs.end(), rhs.m_blobs.begin()
	);
}

} // end of namespace hrb
