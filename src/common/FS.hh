/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/17/18.
//

#pragma once

#if false//__has_include(<filesystem>)
#include <filesystem>

namespace hrb {
namespace fs = std::filesystem;
}
#else

#include <boost/filesystem.hpp>
#include <system_error>

// wrappers for std::error_code -> boost::error_code
// injected to boost::filesystem to make namespace-dependent lookup works
namespace boost::filesystem {

void rename(const path& src, const path& dest, std::error_code& ec);
void create_directories(const path& dir, std::error_code& ec);

}

namespace hrb {
namespace fs = boost::filesystem;
}
#endif
