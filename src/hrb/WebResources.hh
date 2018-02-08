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

#include "util/MMap.hh"

#include <boost/filesystem/path.hpp>
#include <unordered_map>

namespace hrb {

class WebResources
{
public:
	explicit WebResources(const boost::filesystem::path& web_root);

	std::string_view find_static(const std::string& filename) const;
	std::string_view find_dynamic(const std::string& filename) const;

private:
	const std::unordered_map<std::string, MMap>   m_static;
	const std::unordered_map<std::string, MMap>   m_dynamic;
};

} // end of namespace hrb
