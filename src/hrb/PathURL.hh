/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#pragma once

#include <boost/utility/string_view.hpp>

namespace hrb {

// An URL containing a container path, i.e. /view/path/to/dir or /upload/path/to/dir/file.toupload
class PathURL
{
public:
	PathURL(boost::string_view target);

	std::string_view action() const {return m_action;}
	std::string_view path() const {return m_path.empty() ? std::string_view{"/"} : m_path;}
	std::string_view filename() const {return m_filename;}

private:
	// "view" or "upload"
	std::string_view m_action;

	// path to the container
	std::string_view m_path;

	// filename
	std::string_view m_filename;
};

} // end of namespace hrb
