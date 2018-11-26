/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 11/26/18.
//

#include "FS.hh"
#include <boost/system/error_code.hpp>

namespace boost::filesystem {

void rename(const path& src, const path& dest, std::error_code& ec)
{
	boost::system::error_code bec;
	rename(src, dest, bec);

	ec.assign(bec.value(), bec.category());
}

void create_directories(const path& dir, std::error_code& ec)
{
	boost::system::error_code bec;
	create_directories(dir, bec);

	ec.assign(bec.value(), bec.category());
}

} // end of namespace
