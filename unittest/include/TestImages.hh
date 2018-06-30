/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/25/18.
//

#pragma once

#include "common/FS.hh"
#include <vector>

namespace hrb {

	/// Location of the test images in the source code directory.
	/// It is defined by a .cc file in the same directory as the test images.
	extern const fs::path test_images;

	std::vector<unsigned char> random_lena();
}