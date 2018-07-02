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

	auto optimal = subject.square_crop();
	assert(optimal.cols == optimal.rows);

	cv::imwrite("out.jpeg", optimal);

	// resize smaller if it's too big
	auto xratio = 2048 / static_cast<double>(optimal.cols);
	auto yratio = 2048 / static_cast<double>(optimal.rows);

	cv::Mat out;
	if (xratio < 1.0 || yratio < 1.0)
		cv::resize(optimal, out, {}, std::min(xratio, yratio), std::min(xratio, yratio), cv::INTER_LINEAR);
	else
		out = std::move(optimal);

	cv::namedWindow( "imshow", cv::WINDOW_AUTOSIZE );
	cv::imshow("imshow", out);
	cv::waitKey(0);
	return 0;
}