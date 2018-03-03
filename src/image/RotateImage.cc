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
#include "TurboBuffer.hh"
#include "EXIF2.hh"

#include "util/Log.hh"

#include <boost/beast/core/file.hpp>

namespace hrb {

RotateImage::~RotateImage()
{
	tjDestroy(m_transform);
}

TurboBuffer RotateImage::rotate(long orientation, const void *data, std::size_t size)
{
	TurboBuffer buf;

	while (orientation != 1)
	{
		// Allow imperfect transform and trim the residual pixels that cannot
		// be transformed perfectly. It will reduce the image size by a bit,
		// but all sizes of images can be rotated this way.
		tjtransform op{};
		op.op = map_op(orientation);
		op.options = TJXOPT_TRIM;

		unsigned char *out{};
		unsigned long out_size{};
		auto transform_result = tjTransform(
			m_transform,
			buf.empty() ? static_cast<const unsigned char*>(data) : buf.data(),
			buf.empty() ? size : buf.size(),
			1, &out, &out_size, &op, 0
		);
		buf = TurboBuffer{out, out_size};

		if (transform_result != 0)
			return {};
	}

	return buf;
}

int RotateImage::map_op(long& orientation)
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

/// Rotate the image according to the Exif.Image.Orientation tag.
TurboBuffer RotateImage::auto_rotate(const void *jpeg, std::size_t size, std::error_code& ec)
{
	EXIF2 exif2{static_cast<const unsigned char *>(jpeg), size, ec};
	if (ec)
	{
		// nothing to do if no EXIF tag
		if (ec == EXIF2::Error::not_found)
			ec.assign(0, ec.category());

		return {};
	}

	if (auto orientation = exif2.get(static_cast<const unsigned char *>(jpeg), EXIF2::Tag::orientation))
	{
		auto buf = rotate(orientation->value_offset, jpeg, size);
		if (buf.empty())
		{
			Log(LOG_NOTICE, "cannot rotate image %1%", tjGetErrorStr());
			ec.assign(-1, std::system_category());
			return {};
		}

		// Update orientation tag
		orientation->value_offset = 1;

		// Write the new tags to the newly rotated file
		EXIF2 out_exif2{buf.data(), buf.size(), ec};
		out_exif2.set(buf.data(), *orientation);

		return buf;
	}
	else
	{
		return {};
	}
}

} // end of namespace hrb
