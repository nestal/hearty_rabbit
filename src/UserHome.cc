/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 1/11/2021.
//

#include "UserHome.hh"

#include <cassert>

namespace hrb {

UserHome::UserHome(const std::filesystem::path& dir) : m_user_directory{canonical(dir)}
{
	assert(dir.is_absolute());
	assert(is_directory(dir));
}


std::vector<DirectoryEntry> UserHome::list_directory(const std::filesystem::path& path) const
{
	assert(path.is_relative());

	auto normal_path = (m_user_directory/path).lexically_normal();

	auto[end, nothing] = std::mismatch(m_user_directory.begin(), m_user_directory.end(), normal_path.begin());
	if (end != m_user_directory.end())
		throw std::system_error{std::make_error_code(std::errc::no_such_file_or_directory)};

	std::vector<DirectoryEntry> result;
	for (auto&& entry : std::filesystem::directory_iterator{m_user_directory/path})
	{
		result.emplace_back(entry.path().filename().string(), "", ObjectID{});
	}

	return result;
}

} // end of namespace hrb
