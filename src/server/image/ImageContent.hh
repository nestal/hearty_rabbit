/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/30/18.
//

#pragma once

#include <boost/range/iterator_range.hpp>

#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>

namespace hrb {

struct SquareCropSetting;

class ImageContent
{
public:
	using rect_iterator = std::vector<cv::Rect>::const_iterator;

public:
	explicit ImageContent(const cv::Mat& image, const SquareCropSetting& setting);

	boost::iterator_range<rect_iterator>  faces()    const {return {m_faces.begin(), m_faces.end()};}
	boost::iterator_range<rect_iterator>  eyes()     const {return {m_eyes.begin(), m_eyes.end()};}
	boost::iterator_range<rect_iterator>  features() const {return {m_features.begin(), m_features.end()};}

	cv::Rect square_crop() const;

private:
	struct InflectionPoint
	{
		int pos;        // position of the inflection point in the principal axis
		int score;      // represents how important is the rectangle
		int total;      // total score if this inflection point is chosen as the start of the crop
	};

	void add_content(std::vector<InflectionPoint>& infections, const cv::Rect& content, int score_ratio) const;

private:
	cv::Mat m_image;

	std::vector<cv::Rect>       m_faces, m_eyes;
	std::vector<cv::Rect>       m_features;
};

} // end of namespace hrb
