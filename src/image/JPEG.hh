/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/28/18.
//

#pragma once

#include "util/BufferView.hh"
#include <opencv2/core.hpp>

namespace hrb {

cv::Mat load_image(BufferView raw);

} // end of namespace hrb
