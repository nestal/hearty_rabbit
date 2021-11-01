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

namespace hrb {

std::vector<DirectoryEntry> UserHome::list_directory(const std::filesystem::path& path) const
{
	std::vector<DirectoryEntry> result;
	for (auto&& entry : std::filesystem::directory_iterator{path})
	{
		result.emplace_back(entry.path().filename().string(), "", ObjectID{});
	}

	return result;
}

} // end of namespace hrb
