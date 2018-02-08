/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/8/18.
//

#include "WebResources.hh"

#include "ResourcesList.hh"

namespace hrb {
namespace {

template <typename Iterator>
auto load(const boost::filesystem::path& base, Iterator first, Iterator last)
{
	std::error_code ec;
	std::unordered_map<std::string, MMap> result;
	for (auto it = first; it != last && !ec; ++it)
		result.emplace(*it, MMap::open(base / *it, ec));
	return result;
}

} // end of anonymous namespace

WebResources::WebResources(const boost::filesystem::path& web_root) :
	m_static {load(web_root/"static",  static_resources.begin(),  static_resources.end())},
	m_dynamic{load(web_root/"dynamic", dynamic_resources.begin(), dynamic_resources.end())}
{
}

std::string_view WebResources::find_static(const std::string& filename) const
{
	auto it = m_static.find(filename);
	return it != m_static.end() ? it->second.string() : std::string_view{};
}

std::string_view WebResources::find_dynamic(const std::string& filename) const
{
	auto it = m_dynamic.find(filename);
	return it != m_dynamic.end() ? it->second.string() : std::string_view{};
}

} // end of namespace hrb
