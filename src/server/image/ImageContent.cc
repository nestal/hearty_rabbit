/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/30/18.
//

#include "ImageContent.hh"

#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

namespace hrb {

ImageContent::ImageContent(cv::Mat&& image)
{
	if (!m_face_detect.load("/usr/share/opencv/haarcascade/haarcascade_frontalface_default.xml"))
		throw -1;

	cv::Mat frame_gray;
	cv::cvtColor( image, frame_gray, cv::COLOR_BGR2GRAY );

	std::vector<cv::Rect> faces;
}

} // end of namespace hrb
