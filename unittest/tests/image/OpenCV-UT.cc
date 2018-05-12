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

using namespace hrb;

TEST_CASE("open image with opencv", "[normal]")
{
	auto mat = cv::imread((fs::path{__FILE__}.parent_path()/"up_f_rot90.jpg").string(), cv::IMREAD_COLOR);
	REQUIRE(mat.rows == 192);
	REQUIRE(mat.cols == 160);
}
