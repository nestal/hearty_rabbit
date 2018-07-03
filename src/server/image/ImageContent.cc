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

namespace hrb {

ImageContent::ImageContent(const cv::Mat& image) : m_image{image}
{
	// TODO: better error handling
	if (!m_face_detect.load(std::string{constants::haarcascades_path}))
		throw -1;

	// convert to gray and equalize
	cv::Mat gray;
	cvtColor(m_image, gray, cv::COLOR_BGR2GRAY);
	equalizeHist(gray, gray);

	// detect faces
	m_face_detect.detectMultiScale(gray, m_faces, 1.1, 2, cv::CASCADE_SCALE_IMAGE, cv::Size{100, 100} );

	// detect features
	cv::goodFeaturesToTrack(gray, m_features, 200, 0.05, 20);
}

cv::Rect ImageContent::square_crop() const
{
	auto aspect_ratio  = static_cast<double>(m_image.cols) / m_image.rows;
	auto window_length = std::min(m_image.cols, m_image.rows);

	// convert all faces into inflection points by projecting them into
	// the principal axis
	std::vector<InflectionPoint> infections;
	for (auto&& face : m_faces)
		add_content(infections, face);

	for (auto&& feature : m_features)
		add_content(infections, cv::Rect{feature.x, feature.y, 0, 0});

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
void ImageContent::add_content(std::vector<ImageContent::InflectionPoint>& infections, const cv::Rect& content) const
{
	auto window_size = std::min(m_image.cols, m_image.rows);
	auto score = std::max(content.width * content.height, 1);

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
