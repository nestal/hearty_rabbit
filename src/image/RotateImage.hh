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

#include "util/FS.hh"
#include "util/BufferView.hh"

#include <turbojpeg.h>
#include <memory>
#include <tuple>

namespace hrb {

class TurboBuffer;

class RotateImage
{
public:
	RotateImage() = default;
	~RotateImage();

	TurboBuffer rotate(long orientation, BufferView blob, std::error_code& ec);

	TurboBuffer auto_rotate(BufferView blob, std::error_code& ec);

private:
	static int map_op(long& orientation);

private:
	tjhandle m_transform{tjInitTransform()};
};

} // end of namespace hrb
