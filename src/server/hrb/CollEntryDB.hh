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

#include <json.hpp>
#include <string>

namespace hrb {

struct CollEntry;

// along with from_json(CollEntryFields) and to_json(CollEntryFields), CollEntryFields should
// be moved to a separate header

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

	std::string filename() const;
	std::string mime() 	const;
	Timestamp timestamp() const;
	CollEntry fields() const;

	std::string_view json() const;
	Permission permission() const;

	std::string_view raw() const {return m_raw;}
	auto data() const {return m_raw.data();}
	auto size() const {return m_raw.size();}

private:
	std::string_view	m_raw{" {}"};
};

} // end of namespace hrb
