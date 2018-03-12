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

// An URL containing a container path, i.e. /view/user/path/to/dir or /upload/user/path/to/dir/file.toupload
class PathURL
{
public:
	explicit PathURL(boost::string_view target);
	PathURL(std::string_view action, std::string_view user, std::string_view coll, std::string_view name);

	std::string_view action() const {return m_action;}
	std::string_view user() const {return m_user;}
	std::string_view collection() const {return m_coll;}
	std::string_view filename() const {return m_filename;}

	std::string str() const;

private:
	// "view" or "upload"
	std::string_view m_action;

	std::string_view m_user;

	// container name
	std::string_view m_coll;

	// filename
	std::string_view m_filename;
};

} // end of namespace hrb
