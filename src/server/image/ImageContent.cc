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
#include <iostream>

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
	struct InflectionPoint
	{
		int pos;        // position of the inflection point in the principal axis
		int score;      // represents how important is the rectangle
		int total;      // total score if this inflection point is chosen as the start of the crop
	};

	auto aspect_ratio  = static_cast<double>(m_image.cols) / m_image.rows;
	auto window_length = std::min(m_image.cols, m_image.rows);
	auto axis_length   = std::max(m_image.cols, m_image.rows);

	// convert all faces into inflection points by projecting them into
	// the principal axis
	std::vector<InflectionPoint> infections;
	for (auto&& face : m_faces)
	{
		// enter rectangular region: score increases
		infections.push_back(InflectionPoint{
			(aspect_ratio > 1.0 ? face.x + face.width : face.y + face.height) - window_length,
			face.width * face.height
		});

		infections.push_back(InflectionPoint{
			aspect_ratio > 1.0 ? face.x : face.y,
			-face.width * face.height
		});
	}

	// sort the inflect points in the order of their appearance in the principal axis
	std::sort(infections.begin(), infections.end(), [](auto& p1, auto& p2){return p1.pos < p2.pos;});

	// calculate the total score of each inflection point
	auto score = 0;
	for (auto&& pt : infections)
	{
		score += pt.score;
		pt.total = score;
	}

	// ignore the points outside the image (i.e. pos < 0)
	auto start = std::find_if(infections.begin(), infections.end(), [](auto&& p){return p.pos >= 0;});
	infections.erase(infections.begin(), start);

	// find the point with maximum total score
	auto max = std::max_element(infections.begin(), infections.end(), [](auto& p1, auto& p2)
	{
		return p1.total < p2.total;
	});

	assert(max != infections.end());

	std::cout << "found " << max->pos << " " << max->total << std::endl;

	return cv::Mat();
}

} // end of namespace hrb
