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

#include "common/Permission.hh"
#include "common/Timestamp.hh"

#include <nlohmann/json.hpp>

#include <string>
#include <optional>

namespace hrb {

struct CollEntry;

class CollEntryDB
{
public:
	CollEntryDB() = default;
	explicit CollEntryDB(std::string_view redis_reply);

	static std::string create(
		Permission perm, std::string_view filename, std::string_view mime,
		Timestamp timestamp
	);
	static std::string create(Permission perm, const nlohmann::json& json);
	static std::string create(const CollEntry& fields);

	[[nodiscard]] std::string filename() const;
	[[nodiscard]] std::string mime() 	const;
	[[nodiscard]] Timestamp timestamp() const;
	[[nodiscard]] std::optional<CollEntry> fields() const;

	[[nodiscard]] std::string_view json() const;
	[[nodiscard]] Permission permission() const;

	[[nodiscard]] std::string_view raw() const {return m_raw;}
	[[nodiscard]] auto data() const {return m_raw.data();}
	[[nodiscard]] auto size() const {return m_raw.size();}

private:
	std::string_view	m_raw{" {}"};
};

} // end of namespace hrb
