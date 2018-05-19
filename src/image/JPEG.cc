/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/28/18.
//

#include "JPEG.hh"

#include <opencv2/imgcodecs.hpp>
namespace hrb {

cv::Mat load_image(BufferView raw)
{
	return cv::imdecode(
		cv::Mat{1, static_cast<int>(raw.size()), CV_8U, const_cast<unsigned char*>(raw.data())},
		cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH
	);
}

} // end of namespace hrb
