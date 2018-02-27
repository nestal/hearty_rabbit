/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/27/18.
//

#pragma once

#include <turbojpeg.h>
#include <memory>
#include <tuple>

namespace hrb {

class RotateImage
{
public:
	RotateImage() = default;
	~RotateImage();

	std::tuple<
		std::shared_ptr<unsigned char>,
		std::size_t
	> rotate(int orientation, const void *data, std::size_t size);

private:
	static int map_op(int& orientation);

private:
	tjhandle m_transform{tjInitTransform()};
};

} // end of namespace hrb
