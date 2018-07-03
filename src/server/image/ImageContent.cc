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

#include "config.hh"

#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

#include <iostream>

namespace hrb {

ImageContent::ImageContent(const cv::Mat& image) : m_image{image}
{
	if (!m_face_detect.load(std::string{constants::haarcascades_path}))
		throw -1;

	// convert to gray and equalize
	cv::Mat gray;
	cvtColor(m_image, gray, cv::COLOR_BGR2GRAY);
	equalizeHist(gray, gray);

	// detect faces
	m_face_detect.detectMultiScale(gray, m_faces, 1.1, 3, cv::CASCADE_SCALE_IMAGE, cv::Size{m_image.cols/10, m_image.rows/10} );

	// detect features
	cv::goodFeaturesToTrack(gray, m_features, 200, 0.01, 20);
}

cv::Rect ImageContent::square_crop() const
{
	struct InflectionPoint
	{
		int pos;        // position of the inflection point in the principal axis
		int score;      // represents how important is the rectangle
		int total;      // total score if this inflection point is chosen as the start of the crop
	};

	auto aspect_ratio  = static_cast<double>(m_image.cols) / m_image.rows;
	auto window_length = std::min(m_image.cols, m_image.rows);

	// convert all faces into inflection points by projecting them into
	// the principal axis
	std::vector<InflectionPoint> infections;
	for (auto&& face : m_faces)
	{
		std::cout << "face @ " << face.x << " " << face.width << " " << std::endl;
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
	for (auto&& feature : m_features)
	{
		infections.push_back(InflectionPoint{
			(aspect_ratio > 1.0 ? feature.x : feature.y) - window_length,
			1
		});

		infections.push_back(InflectionPoint{
			aspect_ratio > 1.0 ? feature.x : feature.y,
			-1
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

	// aggregate the points outside the image (i.e. pos < 0) into one point at pos = 0
	auto start = std::find_if(infections.begin(), infections.end(), [](auto&& p){return p.pos >= 0;});
	if (start != infections.begin() && start != infections.end())
	{
		start--;
		if (start->pos < 0)
			start->pos = 0;
	}
	infections.erase(infections.begin(), start);

	// find the optimal inflection point with maximum total score
	auto optimal = std::max_element(infections.begin(), infections.end(), [](auto& p1, auto& p2)
	{
		return p1.total < p2.total;
	});

	// deduce ROI rectangle from optimal inflection point
	cv::Rect roi{0, 0, window_length, window_length};
	if (optimal != infections.end())
	{
		std::cout << "found " << optimal->pos << " " << optimal->total << std::endl;
		if (aspect_ratio > 1.0)
			roi.x = optimal->pos;
		else
			roi.y = optimal->pos;
	}
	else
	{
		// crop at center
		if (aspect_ratio > 1.0)
			roi.x = m_image.cols / 2 - window_length/2;
		else
			roi.y = m_image.rows / 2 - window_length/2;
	}
	return roi;
}

} // end of namespace hrb
