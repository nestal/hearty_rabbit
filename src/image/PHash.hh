/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/13/18.
//

#pragma once

#include "util/BufferView.hh"
#include "util/FS.hh"

#include <opencv2/core.hpp>

#include <cstdint>

namespace hrb {

std::uint64_t phash(BufferView image);
std::uint64_t phash(fs::path file);
std::uint64_t phash(const cv::Mat& input);

} // end of namespace
