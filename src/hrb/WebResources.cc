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

#include <boost/exception/info.hpp>

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

const std::string_view dir_needle{"{/** dynamic json placeholder for dir **/}"};
const std::string_view meta_needle{R"(<meta property="og:title" content="Hearty Rabbit">)"};
}

template <typename Iterator>
auto WebResources::load(const boost::filesystem::path& base, Iterator first, Iterator last)
{
	std::error_code ec;
	Container result;
	for (auto it = first; it != last && !ec; ++it)
	{
		auto path = base / *it;
		auto mmap = MMap::open(path, ec);
		if (ec)
			BOOST_THROW_EXCEPTION(Error() << ErrorCode(ec) << MissingResource(path));

		Blake2 hasher;
		hasher.update(mmap.data(), mmap.size());
		auto etag = to_quoted_hex(ObjectID{hasher.finalize()});

		result.emplace(
			*it,
			std::move(mmap),
			std::string{resource_mime(path.extension().string())},
			std::move(etag)
		);
	}
	return result;
}

WebResources::WebResources(const boost::filesystem::path& web_root) :
	m_static {load(web_root/"static",  static_resources.begin(),  static_resources.end())},
	m_dynamic{load(web_root/"dynamic", dynamic_resources.begin(), dynamic_resources.end())}
{
}

WebResources::Response WebResources::find_static(std::string_view filename, boost::string_view etag, int version) const
{
	auto it = m_static.find(filename);
	if (it == m_static.end())
		return Response{http::status::not_found, version};

	if (!etag.empty() && etag == it->etag())
	{
		Response res{http::status::not_modified, version};
		res.set(http::field::cache_control, "private, max-age=0, must-revalidate");
		res.set(http::field::etag, it->etag());
		return res;
	}

	return it->get(version, false);
}

WebResources::Response WebResources::find_dynamic(std::string_view filename, int version) const
{
	auto it = m_dynamic.find(filename);
	return it != m_dynamic.end() ?
		it->get(version, true) :
		Response{http::status::not_found, version};
}

bool WebResources::is_static(std::string_view filename) const
{
	return m_static.find(filename) != m_static.end();
}

WebResources::Response WebResources::inject(http::status status, std::string&& json, std::string&& meta, int version) const
{
	auto res = find_dynamic("index.html", version);
	res.body().set_extra(0, std::move(json));
	res.body().set_extra(1, std::move(meta));
	res.result(status);
	return res;
}

WebResources::Resource::Resource(std::string_view name, MMap&& file, std::string&& mime, boost::string_view etag) :
	m_name{name}, m_file{std::move(file)}, m_mime{std::move(mime)}, m_etag{etag}
{
	if (name == "index.html")
	{
		StringTemplate tmp{m_file.string()};
		tmp.replace(hrb::dir_needle);
		tmp.replace(hrb::meta_needle);
		m_src.assign(tmp.begin(), tmp.end());
	}
	else
	{
		m_src.push_back(m_file.string());
	}
	assert(m_src.size() <= hrb::SplitBuffers::value_type::segment_count());
}

WebResources::Response WebResources::Resource::get(int version, bool dynamic) const
{
	assert(m_src.size() <= hrb::SplitBuffers::value_type::segment_count());
	http::response<hrb::SplitBuffers> result{
		std::piecewise_construct,
		std::make_tuple(m_src.begin(), m_src.end()),
		std::make_tuple(http::status::ok, version)
	};
	result.set(http::field::content_type, m_mime);
	result.set(http::field::cache_control,
		dynamic ? "no-cache, no-store, must-revalidate" : "public, max-age=0, must-revalidate"
	);
	result.set(http::field::etag, m_etag);
	result.prepare_payload();
	return result;
}

} // end of namespace hrb
