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

#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>

namespace hrb {

class ImageContent
{
public:
	explicit ImageContent(cv::Mat&& image);

private:
	cv::CascadeClassifier m_face_detect;
};

} // end of namespace hrb
