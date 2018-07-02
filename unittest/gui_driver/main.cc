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

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace hrb ;

int main(int argc, char **argv)
{
	auto image = cv::imread(argc < 2 ? (test_images/"lena.png").string() : std::string{argv[1]}, cv::IMREAD_COLOR);
	ImageContent subject{image};

	for (auto&& face : subject.faces())
	{
		cv::Point center(face.x + face.width / 2, face.y + face.height / 2);
		ellipse(image, center, cv::Size(face.width / 2, face.height / 2), 0, 0, 360, cv::Scalar(255, 0, 255), 4, 8, 0);
	}

	auto roi = subject.square_crop();
	assert(roi.width == roi.height);
	rectangle(image, roi, cv::Scalar{0, 255, 255}, 10);

	auto xratio = 1024 / static_cast<double>(image.cols);
	auto yratio = 1024 / static_cast<double>(image.rows);

	cv::Mat out;
	if (xratio < 1.0 || yratio < 1.0)
		cv::resize(image, out, {}, std::min(xratio, yratio), std::min(xratio, yratio), cv::INTER_LINEAR);
	else
		out = std::move(image);

	// write cropped image to disk
	if (argc > 2)
		cv::imwrite(argv[2], image(roi));

	std::cout << "width = " << out.cols << " height = " << out.rows << std::endl;

	cv::namedWindow( "imshow", cv::WINDOW_AUTOSIZE );
	cv::imshow("imshow", out);
	cv::waitKey(0);
	return 0;
}