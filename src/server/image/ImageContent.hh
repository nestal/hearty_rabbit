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

class ImageContent
{
public:
	using face_iterator = std::vector<cv::Rect>::const_iterator;

public:
	explicit ImageContent(const cv::Mat& image);

	boost::iterator_range<face_iterator> faces() const {return {m_faces.begin(), m_faces.end()};}

	cv::Mat square_crop() const;

private:
	cv::CascadeClassifier m_face_detect;

	std::vector<cv::Rect> m_faces;
};

} // end of namespace hrb
