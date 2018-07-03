/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 7/2/18.
//

#include "image/ImageContent.hh"
#include "TestImages.hh"

#include <opencv2/imgcodecs.hpp>

#include <catch.hpp>

using namespace hrb;

TEST_CASE("crop at the center if no object is detected", "[normal]")
{
	auto image = cv::imread(fs::path{test_images/"rgb-vertical.jpg"}.string(), cv::IMREAD_COLOR);
	REQUIRE(image.cols == 100);
	REQUIRE(image.rows == 200);

	ImageContent subject{image};

	auto roi = subject.square_crop();
	REQUIRE(roi.width == 100);
	REQUIRE(roi.height == 100);
	REQUIRE(roi.x == 0);
	REQUIRE(roi.y == 50);
}

TEST_CASE("square images will not be cropped", "[normal]")
{
	auto lena = random_lena();
	REQUIRE(lena.cols == lena.rows);

	ImageContent subject{lena};

	auto roi = subject.square_crop();
	REQUIRE(roi.width == roi.height);
	REQUIRE(roi.x == 0);
	REQUIRE(roi.y == 0);
}