/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/12/18.
//

#include <catch2/catch.hpp>

#include "TestImages.hh"

#include "image/PHash.hh"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <bitset>

using namespace hrb;

TEST_CASE("resize 150% up_f_rot90 produce the same phash as original", "[normal]")
{
	auto mat = cv::imread((test::images/"up_f_rot90.jpg").string(), cv::IMREAD_COLOR);
	REQUIRE(mat.rows == 192);
	REQUIRE(mat.cols == 160);

	// enlarge image
	cv::Mat large;
	resize(mat, large, {}, 1.5, 1.5);
	REQUIRE(large.rows == 192*1.5);
	REQUIRE(large.cols == 160*1.5);

	REQUIRE(phash(mat).value() != std::numeric_limits<unsigned long long>::max());
	REQUIRE(phash(mat) == phash(large));
}

TEST_CASE("phash of resized lena should be the same as the original", "[normal]")
{
	auto lena = cv::imread((test::images/"lena.png").string(), cv::IMREAD_COLOR);

	cv::Mat large_2x;
	resize(lena, large_2x, {}, 2, 2);
	REQUIRE(phash(lena) == phash(large_2x));

	cv::Mat large_3x;
	resize(lena, large_3x, {}, 3, 3);
	REQUIRE(phash(lena) == phash(large_3x));
}

TEST_CASE("max phash difference", "[normal]")
{
	PHash min{0ULL}, max{~0ULL};
	INFO("norm is " << min.compare(max));
	REQUIRE(min.compare(max) == 64);
}

TEST_CASE("phash of non-image returns 0", "[normal]")
{
	BufferView non_img{reinterpret_cast<const unsigned char*>("this is not an image")};
	REQUIRE(phash(non_img) == PHash{});
}

TEST_CASE("test PHash ctor", "[normal]")
{
	REQUIRE(PHash{1}.value() == 1);
	REQUIRE(PHash{}.value() == 0);
}
