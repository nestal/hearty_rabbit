/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 7/1/18.
//

#include "image/ImageContent.hh"
#include "TestImages.hh"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

using namespace hrb ;

int main()
{
	auto lena = cv::imread((test_images/"lena.png").string(), cv::IMREAD_COLOR);
	ImageContent subject{lena};

	cv::namedWindow( "imshow", cv::WINDOW_AUTOSIZE );
	cv::imshow("imshow", lena);
	cv::waitKey(0);
	return 0;
}