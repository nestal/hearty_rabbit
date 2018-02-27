/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/27/18.
//

#include "RotateImage.hh"

#include "util/Log.hh"

namespace hrb {

RotateImage::~RotateImage()
{
	tjDestroy(m_transform);
}

std::tuple<
	std::shared_ptr<unsigned char>,
	std::size_t
> RotateImage::rotate(int orientation, const void *data, std::size_t size)
{
	unsigned char *in{};
	unsigned char *out{};
	unsigned long in_size{size}, out_size{};

	while (orientation != 1)
	{
		tjtransform op{};
		op.op = map_op(orientation);
		op.options = TJXOPT_PERFECT;

		auto transform_result = tjTransform(
			m_transform, in ? in : static_cast<const unsigned char*>(data),  in_size,
			1, &out, &out_size,
			&op, 0
		);

		if (transform_result != 0)
			throw -1;

		if (in)
			tjFree(in);

		// Next round
		in      = out;
		in_size = out_size;
	}

	return std::make_tuple(
		std::shared_ptr<unsigned char>(in, [](unsigned char* p){tjFree(p);}),
		out_size
	);
}

int RotateImage::map_op(int& orientation)
{
	// http://sylvana.net/jpegcrop/exif_orientation.html
	//	  1        2       3      4         5            6           7          8
	//
	//	FFFFFF  FFFFFF      FF  FF      FFFFFFFFFF  FF                  FF  FFFFFFFFFF
	//	FF          FF      FF  FF      FF  FF      FF  FF          FF  FF      FF  FF
	//	FFFF      FFFF    FFFF  FFFF    FF          FFFFFFFFFF  FFFFFFFFFF          FF
	//	FF          FF      FF  FF
	//	FF          FF  FFFFFF  FFFFFF
	switch (orientation)
	{
		default:
		case 1: orientation = 1; return TJXOP_NONE;
		case 2: orientation = 1; return TJXOP_HFLIP;
		case 3: orientation = 1; return TJXOP_ROT180;
		case 4: orientation = 2; return TJXOP_ROT180;
		case 5: orientation = 2; return TJXOP_ROT90;
		case 6: orientation = 1; return TJXOP_ROT90;
		case 7: orientation = 2; return TJXOP_ROT270;
		case 8: orientation = 1; return TJXOP_ROT270;
	}
}

} // end of namespace hrb
