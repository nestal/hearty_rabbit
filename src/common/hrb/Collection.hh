/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/6/18.
//

#pragma once

#include "ObjectID.hh"
#include "BlobInode.hh"

#include <boost/range/iterator_range.hpp>

#include <nlohmann/json.hpp>
#include <filesystem>

namespace hrb {

class Blob;

class Collection
{
public:
	using iterator = std::unordered_map<ObjectID, BlobInode>::const_iterator;

public:
	Collection() = default;
	Collection(std::string_view name, std::string_view owner, nlohmann::json&& meta);
	Collection(std::string_view name, std::string_view owner, const ObjectID& cover);
	explicit Collection(const std::filesystem::path& path);

	Collection(Collection&&) = default;
	Collection(const Collection&) = default;
	Collection& operator=(Collection&&) = default;
	Collection& operator=(const Collection&) = default;

	[[nodiscard]] std::string_view name() const {return m_name;}
	[[nodiscard]] std::string_view owner() const {return m_owner;}
	[[nodiscard]] std::optional<ObjectID> cover() const;
	[[nodiscard]] auto begin() const {return m_blobs.begin();}
	[[nodiscard]] auto begin() {return m_blobs.begin();}
	[[nodiscard]] auto end() const {return m_blobs.end();}
	[[nodiscard]] auto end() {return m_blobs.end();}
	[[nodiscard]] iterator find(const ObjectID& id) const;

	[[nodiscard]] std::optional<Blob> get_blob(const ObjectID& id) const;

	friend void from_json(const nlohmann::json& src, Collection& dest);
	friend void to_json(nlohmann::json& dest, const Collection& src);

	[[nodiscard]] nlohmann::json& meta() {return m_meta;}
	[[nodiscard]] const nlohmann::json& meta() const {return m_meta;}

	void add_blob(const ObjectID& id, BlobInode&& entry);
	void add_blob(const ObjectID& id, const BlobInode& entry);

	void remove_blob(const ObjectID& id);

	void update_timestamp(const ObjectID& id, Timestamp value);
	[[nodiscard]] std::size_t size() const {return m_blobs.size();}

	bool operator==(const Collection& rhs) const;
	bool operator!=(const Collection& rhs) const {return !operator==(rhs);}

private:
	std::string     m_name;
	std::string     m_owner;
	nlohmann::json  m_meta;

	std::unordered_map<ObjectID, BlobInode> m_blobs;
};

} // end of namespace hrb
