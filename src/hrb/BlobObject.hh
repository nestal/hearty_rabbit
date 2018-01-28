/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/15/18.
//

#pragma once

#include "util/MMap.hh"

#include <boost/filesystem/path.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <system_error>

namespace hrb {
namespace redis {
class Database;
}

// If use a typedef (or using), then the argument-dependent lookup (ADL) will not
// work for operator<<
struct ObjectID : std::array<unsigned char, 64> {using array::array;};
static_assert(std::is_standard_layout<ObjectID>::value);

class BlobObject
{
public:
	BlobObject() = default;
	explicit BlobObject(const boost::filesystem::path& path);
	BlobObject(std::string_view blob, std::string_view name);
	BlobObject(BlobObject&&) = default;
	BlobObject(const BlobObject&) = delete;
	~BlobObject() = default;

	BlobObject& operator=(BlobObject&&) = default;
	BlobObject& operator=(const BlobObject&) = delete;

	const ObjectID& ID() const {return m_id;}
	bool empty() const {return !m_blob.is_opened();}

	using Completion = std::function<void(BlobObject&, std::error_code ec)>;

	// Load BlobObject from redis database
	static void load(redis::Database& db, const ObjectID& id, Completion completion);
	static void load(
		redis::Database& db,
		const ObjectID& id,
		const boost::filesystem::path& path,
		Completion completion
	);

	void save(redis::Database& db, Completion completion);
	void open(const boost::filesystem::path& path, std::error_code& ec);
	void assign(std::string_view blob, std::string_view name, std::error_code& ec);

	std::string_view blob() const;
	const std::string& name() const {return m_name;}
	const std::string& mime() const {return m_mime;}

private:
	static ObjectID hash(std::string_view blob);
	static std::string deduce_mime(std::string_view blob);
	void open(
		const boost::filesystem::path& path,
		const ObjectID* id,
		std::string_view name,
		std::string_view mime,
		std::error_code& ec
	);
	void assign_field(std::string_view field, std::string_view value);

private:
	ObjectID    m_id;       //!< SHA1 hash of the blob
	std::string m_name;     //!< Typically the file name of the blob
	std::string m_mime;     //!< Mime-type of the blob, deduced by libmagic

	// TODO:    use a variant to support other backing store in addition to memory mappings
	//          e.g. std::string_view (non-owning), std::vector (owning)
	//          also enforce the memory mapping to be read-only
	MMap        m_blob;
};

std::ostream& operator<<(std::ostream& os, const ObjectID& id);

} // end of namespace hrb
