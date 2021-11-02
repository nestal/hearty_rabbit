/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 1/11/2021.
//

#pragma once

#include <hrb/ObjectID.hh>

#include <string>
#include <filesystem>

namespace hrb {

class DirectoryEntry
{
public:
	DirectoryEntry(std::string filename, std::string mime, const ObjectID& id) :
		m_filename{std::move(filename)},
		m_mime{std::move(mime)},
		m_id{id}
	{
	}

	[[nodiscard]] const std::string& filename() const noexcept  {return m_filename;}
	[[nodiscard]] const std::string& mime() const noexcept      {return m_mime;}

private:
	std::string m_filename;
	std::string m_mime;
	ObjectID    m_id;
};

/// Home directory of a user. Contain all the data owned by the user and
/// manages the cache of them.
class UserHome
{
public:
	explicit UserHome(const std::filesystem::path& dir);

	[[nodiscard]] std::vector<DirectoryEntry> list_directory(const std::filesystem::path& path) const;

private:
	std::filesystem::path   m_user_directory;
};

} // end of namespace hrb
