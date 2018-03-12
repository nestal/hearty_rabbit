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

#include "net/Request.hh"

#include "util/MMap.hh"
#include "util/Exception.hh"
#include "util/FS.hh"
#include "net/SplitBuffers.hh"

#include <boost/utility/string_view.hpp>
#include <unordered_map>

namespace hrb {

class WebResources
{
public:
	struct Error : virtual Exception {};
	using MissingResource = boost::error_info<struct missing_resource, fs::path>;

	using Response = http::response<SplitBuffers>;

	explicit WebResources(const fs::path& web_root);

	Response find_static(const std::string& filename, boost::string_view etag, int version) const;
	Response find_dynamic(const std::string& filename, int version) const;

	bool is_static(const std::string& filename) const;

private:
	class Resource
	{
	public:
		Resource(MMap&& file, std::string&& mime, boost::string_view etag) :
			m_file{std::move(file)}, m_mime{std::move(mime)}, m_etag{etag} {}

		http::response<SplitBuffers> get(int version, bool dynamic) const;

		boost::string_view etag() const {return m_etag;}

	private:
		MMap        m_file;
		std::string m_mime;
		std::string m_etag;
	};

	template <typename Iterator>
	static auto load(const fs::path& base, Iterator first, Iterator last);

private:
	const std::unordered_map<std::string, Resource>   m_static;
	const std::unordered_map<std::string, Resource>   m_dynamic;
};

} // end of namespace hrb
