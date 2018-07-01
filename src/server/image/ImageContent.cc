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

ImageContent::ImageContent(const cv::Mat& image) : m_image{image}
{
	if (!m_face_detect.load("/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml"))
		throw -1;

	// convert to gray and equalize
	cv::Mat gray;
	cvtColor(m_image, gray, cv::COLOR_BGR2GRAY);
	equalizeHist(gray, gray);

	// detect faces
	std::vector<cv::Rect> faces;
	m_face_detect.detectMultiScale( gray, faces, 1.1, 2, 0|cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30) );

	m_faces = std::move(faces);
}

cv::Mat ImageContent::square_crop() const
{
	auto aspect_ratio = static_cast<double>(m_image.cols) / m_image.rows;

	// horizontal axis is the principal axis
	// try different points along the axis as the center of our resultant square
	// and see if it can contain all the faces
	if (aspect_ratio > 1.0)
	{

	}

	return cv::Mat();
}

} // end of namespace hrb
