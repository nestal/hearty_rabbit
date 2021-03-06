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

#include <filesystem>

namespace hrb {

namespace fs = std::filesystem;

// polyfill for boost::filesystem::absolute()
fs::path absolute(const fs::path& p, const fs::path& base);

}
