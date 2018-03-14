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

#include <string>
#include <string_view>

namespace hrb {

// TODO: move to a separate header file
class CollEntry
{
public:
	CollEntry() = default;
	explicit CollEntry(std::string_view redis_reply);

	static std::string create(std::string_view perm, std::string_view filename, std::string_view mime);

	bool allow(std::string_view user) const;

	std::string filename() const;
	std::string mime() 	const;

	std::string_view json() const;

	std::string_view raw() const {return m_raw;}
	auto data() const {return m_raw.data();}
	auto size() const {return m_raw.size();}

private:
	std::string_view	m_raw{" {}"};
};

} // end of namespace hrb
