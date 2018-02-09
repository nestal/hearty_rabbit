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
std::string_view resource_mime(const std::string& ext)
{
	// don't expect a big list
	if (ext == ".html") return "text/html";
	else if (ext == ".css") return "text/css";
	else if (ext == ".svg") return "image/svg+xml";
	else if (ext == ".js") return "application/javascript";
	else return "application/octet-stream";
}
}

template <typename Iterator>
auto WebResources::load(const boost::filesystem::path& base, Iterator first, Iterator last)
{
	std::error_code ec;
	std::unordered_map<std::string, Resource> result;
	for (auto it = first; it != last && !ec; ++it)
	{
		auto path = base / *it;
		result.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(*it),
			std::forward_as_tuple(
				MMap::open(path, ec),
				std::string{resource_mime(path.extension().string())}
			)
		);
	}
	return result;
}

WebResources::WebResources(const boost::filesystem::path& web_root) :
	m_static {load(web_root/"static",  static_resources.begin(),  static_resources.end())},
	m_dynamic{load(web_root/"dynamic", dynamic_resources.begin(), dynamic_resources.end())}
{
}

http::response<FileBuffers> WebResources::find_static(const std::string& filename, int version) const
{
	auto it = m_static.find(filename);
	return it != m_static.end() ?
		it->second.get(version) :
		http::response<FileBuffers>{http::status::not_found, version};
}

http::response<FileBuffers> WebResources::find_dynamic(const std::string& filename, int version) const
{
	auto it = m_dynamic.find(filename);
	return it != m_dynamic.end() ?
		it->second.get(version) :
		http::response<FileBuffers>{http::status::not_found, version};
}

http::response<FileBuffers> WebResources::Resource::get(int version) const
{
	http::response<hrb::FileBuffers> result{
		std::piecewise_construct,
		std::make_tuple(m_file.string()),
		std::make_tuple(http::status::ok, version)
	};
	result.set(http::field::content_type, m_mime);
	result.set(http::field::cache_control, "no-cache, no-store, must-revalidate");
	return result;
}
} // end of namespace hrb
