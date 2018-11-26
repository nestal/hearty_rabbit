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
#include "common/FS.hh"

#include "config.hh"

#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>

namespace hrb {

ImageContent::ImageContent(const cv::Mat& image, const fs::path& haar_path) : m_image{image}
{
	// TODO: better error handling
	cv::CascadeClassifier face_detect, eye_detect;
	if (!face_detect.load((haar_path/"haarcascade_frontalface_default.xml").string()))
		throw std::runtime_error("cannot load frontalface model from " + (haar_path/"haarcascade_frontalface_default.xml").string());
	if (!eye_detect.load((haar_path/"haarcascade_eye_tree_eyeglasses.xml").string()))
		throw std::runtime_error("cannot load eye model from " + (haar_path/"haarcascade_eye_tree_eyeglasses.xml").string());

	// convert to gray and equalize
	cv::Mat gray;
	cvtColor(m_image, gray, cv::COLOR_BGR2GRAY);
	equalizeHist(gray, gray);

	// detect faces
	std::vector<cv::Rect> potential_faces;
	face_detect.detectMultiScale(gray, potential_faces, 1.1, 2, cv::CASCADE_SCALE_IMAGE, cv::Size{m_image.cols/20, m_image.rows/20});

	for (auto&& rect : potential_faces)
	{
		auto face_mat = m_image(rect);

		std::vector<cv::Rect> eyes;
		eye_detect.detectMultiScale(face_mat, eyes, 1.1, 2, cv::CASCADE_SCALE_IMAGE, cv::Size{rect.width/5, rect.height/5});

		for (auto&& eye : eyes)
			m_eyes.emplace_back(eye.x + rect.x, eye.y + rect.y, eye.width, eye.height);

		// treat faces without eyes as feature only
		if (eyes.empty())
			m_features.push_back(rect);
		else
			m_faces.push_back(rect);

	}

	// detect features
	std::vector<cv::Point> features;
	cv::goodFeaturesToTrack(gray, features, 200, 0.05, 20);
	for (auto&& feature : features)
		m_features.emplace_back(feature.x, feature.y, 0, 0);
}

cv::Rect ImageContent::square_crop() const
{
	auto window_length = std::min(m_image.cols, m_image.rows);

	// convert all faces into inflection points by projecting them into
	// the principal axis
	std::vector<InflectionPoint> infections;
	for (auto&& face : m_faces)
		add_content(infections, face, 1);

	for (auto&& feature : m_features)
		add_content(infections, feature, 0);

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
		if (m_image.cols > m_image.rows)
			roi.x = optimal->pos;
		else
			roi.y = optimal->pos;
	}
	else
	{
		// crop at center
		if (m_image.cols > m_image.rows)
			roi.x = m_image.cols / 2 - window_length/2;
		else
			roi.y = m_image.rows / 2 - window_length/2;
	}
	return roi;
}

/// Create two infection pints for each content rectangle.
///
/// The first infection point is the point where the content rectangle enters the sliding window. That means the
/// content rectangle will start appearing in the cropped image, and so the score should be awarded. As shown in the
/// diagram below, the position of this inflection point is rect.x + rect.width - window_size, where "rect" is the
/// content rectangle's position.
///
/// \code
///        rect.x + rect.width - window size
///        |                              rect.x + rect.width
///        |                              |
///        v                              v
/// /-------------------------------------------------------------\
/// |      |                              |                       |
/// |      |            /-----------------\                       |
/// |      |            |                 |                       |
/// |      |            |    content      |                       |
/// |      |            |     rect        |                       |
/// |      |            |                 |                       |
/// |      |            \-----------------/                       |
/// |      |                              |                       |
/// |      |<------ Sliding Window------->|                       |
/// |      ^                              |                       |
/// |    Enter Window (add score)         |                       |
/// |      |                              |                       |
/// \-------------------------------------------------------------/
/// \endcode
///
/// The second infection point is the point where the content rectangle exits the sliding window. This point is at
/// rect.x. The score of the content rectangle will be subtracted.
///
/// \code
///                     rect.x
///                     |
///                     v
/// /-------------------------------------------------------------\
/// |                   |                 |            |          |
/// |                   /-----------------\            |          |
/// |                   |                 |            |          |
/// |                   |    content      |            |          |
/// |                   |     rect        |            |          |
/// |                   |                 |            |          |
/// |                   \-----------------/            |          |
/// |                   |                              |          |
/// |                   |<------ Sliding Window------->|          |
/// |                   ^                              |          |
/// |                 Exit Window (subtract score)     |          |
/// |                   |                              |          |
/// \-------------------------------------------------------------/
/// \endcode
///
/// \param infections
/// \param content
void ImageContent::add_content(std::vector<ImageContent::InflectionPoint>& infections, const cv::Rect& content, int score_ratio) const
{
	auto window_size = std::min(m_image.cols, m_image.rows);
	auto score = std::max(score_ratio * content.width * content.height, 1);

	// enter rectangular region: score increases
	infections.push_back(InflectionPoint{
		(m_image.cols > m_image.rows ? content.x + content.width : content.y + content.height) - window_size,
		score
	});

	// exit rectangular regio: score decreases
	infections.push_back(InflectionPoint{
		m_image.cols > m_image.rows ? content.x : content.y,
		-score
	});
}

} // end of namespace hrb
