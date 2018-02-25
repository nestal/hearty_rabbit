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

#include "ObjectID.hh"
#include "ResourcesList.hh"
#include "crypto/Blake2.hh"

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
		auto mmap = MMap::open(path, ec);

		Blake2 hasher;
		hasher.update(mmap.data(), mmap.size());
		auto etag = to_hex(ObjectID{hasher.finalize()});

		result.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(*it),
			std::forward_as_tuple(
				std::move(mmap),
				std::string{resource_mime(path.extension().string())},
				std::move(etag)
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

WebResources::Response WebResources::find_static(const std::string& filename, boost::string_view etag, int version) const
{
	auto it = m_static.find(filename);
	if (it == m_static.end())
		return Response{http::status::not_found, version};

	if (!etag.empty() && etag == it->second.etag())
		return Response{http::status::not_modified, version};

	return it->second.get(version, false);
}

WebResources::Response WebResources::find_dynamic(const std::string& filename, int version) const
{
	auto it = m_dynamic.find(filename);
	return it != m_dynamic.end() ?
		it->second.get(version, true) :
		Response{http::status::not_found, version};
}

WebResources::Response WebResources::Resource::get(int version, bool dynamic) const
{
	http::response<hrb::SplitBuffers> result{
		std::piecewise_construct,
		std::make_tuple(m_file.string()),
		std::make_tuple(http::status::ok, version)
	};
	result.set(http::field::content_type, m_mime);
	result.set(http::field::cache_control,
		dynamic ? "no-cache, no-store, must-revalidate" : "public, max-age=0, must-revalidate"
	);
	result.set(http::field::etag, m_etag);
	return result;
}
} // end of namespace hrb
