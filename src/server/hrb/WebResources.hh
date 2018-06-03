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
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <functional>

namespace hrb {

class WebResources
{
public:
	struct Error : virtual Exception {};
	using MissingResource = boost::error_info<struct missing_resource, fs::path>;

	using Response = http::response<SplitBuffers>;

	explicit WebResources(const fs::path& web_root);

	Response find_static(std::string_view filename, boost::string_view etag, int version) const;
	Response inject(http::status status, std::string&& json, std::string&& meta, int version) const;

	bool is_static(std::string_view filename) const;

private:
	Response find_dynamic(std::string_view filename, int version) const;
	class Resource
	{
	public:
		Resource(std::string_view name, MMap&& file, std::string&& mime, boost::string_view etag);

		http::response<SplitBuffers> get(int version, bool dynamic) const;

		boost::string_view etag() const {return m_etag;}
		std::string_view name() const {return m_name;}

	private:
		std::string m_name;

		MMap        m_file;
		std::vector<std::string_view> m_src;

		std::string m_mime;
		std::string m_etag;
	};

	template <typename Iterator>
	static auto load(const fs::path& base, Iterator first, Iterator last);

private:
	using Container = boost::multi_index_container<
		Resource,
		boost::multi_index::indexed_by<
			boost::multi_index::hashed_unique<
				boost::multi_index::const_mem_fun<
					Resource,
					std::string_view,
					&Resource::name
				>,
				std::hash<std::string_view>
			>
		>
	>;

	const Container   m_static;
	const Container   m_dynamic;
};

} // end of namespace hrb
