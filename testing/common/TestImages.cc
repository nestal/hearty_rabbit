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

#include <random>

// http://engineering.curalate.com/2017/04/13/content-based-intelligent-cropping.html

namespace hrb {

const std::filesystem::path test_images{std::filesystem::path{__FILE__}.parent_path()};

cv::Mat random_lena()
{
	thread_local std::mt19937_64 mt{std::random_device{}()};

	auto lena = cv::imread((test_images/"lena.png").string(), cv::IMREAD_COLOR);

	std::uniform_int_distribution<> dis{16, 64};

	cv::Mat noise{lena.size(), CV_8UC3};
	randn(noise, 0, dis(mt));
	lena += noise;

	return lena;
}

std::vector<unsigned char> random_lena_bytes()
{
	auto lena = random_lena();

	std::vector<unsigned char> out_buf;
	cv::imencode(".jpg", lena, out_buf, {cv::IMWRITE_JPEG_QUALITY, 50});
	return out_buf;
}

} // end of namespace
