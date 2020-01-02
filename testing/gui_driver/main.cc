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
#include "common/util/FS.hh"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <boost/filesystem.hpp>

#include <iostream>

#include <config.hh>

using namespace hrb ;

void proc_image(const fs::path& path)
{
	auto image = cv::imread(path.string(), cv::IMREAD_COLOR);
	if (image.empty())
		return;

	ImageContent subject{image, std::string{constants::haarcascades_path}};

	for (auto&& face : subject.faces())
	{
		cv::Point center(face.x + face.width / 2, face.y + face.height / 2);
		ellipse(image, center, cv::Size(face.width / 2, face.height / 2), 0, 0, 360, cv::Scalar(255, 0, 255), 4, 8, 0);
	}

	for (auto&& eye : subject.eyes())
	{
		cv::Point center(eye.x + eye.width / 2, eye.y + eye.height / 2);
		ellipse(image, center, cv::Size(eye.width / 2, eye.height / 2), 0, 0, 360, cv::Scalar(0, 0, 0), 4, 8, 0);
	}

	for (auto&& feature : subject.features())
	{
		ellipse(image, {feature.x, feature.y}, cv::Size{5, 5}, 0, 0, 360, cv::Scalar(0, 0, 255), 4, 8, 0);
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

	std::cout << "width = " << out.cols << " height = " << out.rows << std::endl;

	cv::namedWindow( "imshow", cv::WINDOW_AUTOSIZE );
	cv::imshow("imshow", out);
	cv::waitKey(0);
}

int main(int argc, char **argv)
{
	fs::directory_iterator di{fs::current_path()};
	for (auto&& path : di)
	{
		std::cout << path.path() << " " << path.path().extension() << std::endl;
		if (path.path().extension() == ".jpeg" || path.path().extension() == ".jpg")
			proc_image(path.path());
	}
	return 0;
}
