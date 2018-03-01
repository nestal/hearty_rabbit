/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 1/Mar/18.
//

#pragma once

#include <boost/endian/buffers.hpp>

#include <cstddef>

namespace hrb {

class Exif2
{
public:
	Exif2(const unsigned char *jpeg, std::size_t size);

private:

};

} // end o namespace
