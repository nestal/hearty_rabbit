/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/8/18.
//

#pragma once

#include "Request.hh"

#include "util/MMap.hh"
#include "net/FileBuffers.hh"

#include <boost/filesystem/path.hpp>
#include <unordered_map>

namespace hrb {

class WebResources
{
public:
	explicit WebResources(const boost::filesystem::path& web_root);

	http::response<FileBuffers> find_static(const std::string& filename, int version) const;
	http::response<FileBuffers> find_dynamic(const std::string& filename, int version) const;

private:
	class Resource
	{
	public:
		Resource(MMap&& file, std::string&& mime) : m_file{std::move(file)}, m_mime{std::move(mime)} {}

		http::response<FileBuffers> get(int version) const;

	private:
		MMap        m_file;
		std::string m_mime;
	};

	template <typename Iterator>
	static auto load(const boost::filesystem::path& base, Iterator first, Iterator last);

private:
	const std::unordered_map<std::string, Resource>   m_static;
	const std::unordered_map<std::string, Resource>   m_dynamic;
};

} // end of namespace hrb
