/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/25/18.
//

#include "TestImages.hh"

#include <opencv2/imgcodecs.hpp>

namespace hrb {

const fs::path test_images{fs::path{__FILE__}.parent_path()};

std::vector<unsigned char> random_lena()
{
	auto lena = cv::imread((test_images/"lena.png").string(), cv::IMREAD_COLOR);
	randn(lena, cv::Mat{50, 50, 50}, cv::Mat{50, 50, 50});

	std::vector<unsigned char> out_buf;
	cv::imencode(".jpg", lena, out_buf, {cv::IMWRITE_JPEG_QUALITY, 50});
	return out_buf;
}

} // end of namespace
