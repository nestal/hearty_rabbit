/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/12/18.
//

#include <catch.hpp>

#include "util/FS.hh"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <bitset>

using namespace hrb;

std::uint64_t phash(cv::Mat image)
{
	// gray
	if (image.channels() > 1)
		cv::cvtColor(cv::Mat{image}, image, CV_BGR2GRAY);

	std::bitset<64> hash;

	cv::Mat img32;
	resize(image, img32, {32, 32});
	img32 = cv::Mat_<double>(img32);

	cv::Mat dst;
	dct(img32, dst);

	dst = dst({1, 1, 8, 8});
	auto mean_mat = mean(dst);

	cv::Mat mask = (dst >= mean_mat[0]);
	for (int i = 0; i<mask.rows; i++)
		for (int j = 0; j<mask.cols; j++)
			mask.at<uchar>(i, j) == 0 ?
			    (hash[i*mask.cols + j] = false) :
		        (hash[i*mask.cols + j] = true);

	return hash.to_ullong();
}

TEST_CASE("open image with opencv", "[normal]")
{
	auto mat = cv::imread((fs::path{__FILE__}.parent_path()/"up_f_rot90.jpg").string(), cv::IMREAD_COLOR);
	REQUIRE(mat.rows == 192);
	REQUIRE(mat.cols == 160);

	// enlarge image
	cv::Mat large;
	resize(mat, large, {}, 1.5, 1.5);
	REQUIRE(large.rows == 192*1.5);
	REQUIRE(large.cols == 160*1.5);

	REQUIRE(phash(mat) != std::numeric_limits<unsigned long long>::max());
	REQUIRE(phash(mat) == phash(large));
}

TEST_CASE("phash of resized lena should be the same as the original", "[normal]")
{
	auto lena = cv::imread((fs::path{__FILE__}.parent_path()/"lena.png").string(), cv::IMREAD_COLOR);
	cv::Mat large;

	resize(lena, large, {}, 2, 2);
	REQUIRE(phash(lena) == phash(large));
}
