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
#include "TurboBuffer.hh"

#include "util/Log.hh"

#include <turbojpeg.h>
#include <memory>

namespace hrb {
namespace {
const int yuv_pad = 2;
}

using Handle = std::unique_ptr<void, decltype(&tjDestroy)>;

JPEG::JPEG(const void *jpeg_data, std::size_t jpeg_size, const Size2D& max_dim)
{
	Handle handle{tjInitDecompress(), &tjDestroy};
	auto result = tjDecompressHeader3(
		handle.get(), static_cast<const unsigned char*>(jpeg_data), jpeg_size,
		&m_size.width(), &m_size.height(), &m_subsample, &m_colorspace
	);

	if (result != 0)
		throw Exception(tjGetErrorStr());

	auto selected_size = select_scaling_factor(max_dim, m_size);
	Log(LOG_NOTICE, "Resizing JPEG from %1% to %2%", m_size, selected_size);

	// allocate yuv planar buffer
	std::vector<unsigned char> yuv(tjBufSizeYUV2(selected_size.width(), yuv_pad, selected_size.height(), m_subsample));

	result = tjDecompressToYUV2(
		handle.get(), static_cast<const unsigned char*>(jpeg_data), jpeg_size,
		&yuv[0], selected_size.width(), yuv_pad, selected_size.height(), 0
	);
	if (result != 0)
		throw Exception(tjGetErrorStr());

	// commit result
	m_yuv  = std::move(yuv);
	m_size = selected_size;
}

// width, height: target size of the image.
Size2D JPEG::select_scaling_factor(const Size2D& max, const Size2D& actual)
{
	Size2D selected, minimum = actual;
	if (actual.width() > max.width() || actual.height() > max.height())
	{
		int count{};
		auto factors = tjGetScalingFactors(&count);
		for (int i = 0; i < count; i++)
		{
			auto w = TJSCALED(actual.width(),  factors[i]);
			auto h = TJSCALED(actual.height(), factors[i]);

			// width and height within limits, and total area larger
			// than precious accepted values
			if (
				w <= actual.width() && h <= actual.height() &&
				w <= max.width()    && h <= max.height()    &&
				w*h > selected.area()
			)
			{
				// remember this scaling factor
				selected.assign(w, h);
			}

			if (w*h < minimum.area())
				minimum.assign(w, h);
		}
	}

	return selected.area() == 0 ? minimum : selected;
}

TurboBuffer JPEG::compress(int quality) const
{
	Handle handle{tjInitCompress(), &tjDestroy};

	unsigned char *jpeg{};
	std::size_t size{};
	auto result = tjCompressFromYUV(
		handle.get(), &m_yuv[0], m_size.width(), yuv_pad, m_size.height(), m_subsample, &jpeg, &size,
		quality, TJFLAG_FASTDCT
	);
	if (result != 0)
		throw Exception(tjGetErrorStr());

	return {jpeg, size};
}

JPEG::JPEG(JPEG&& src) noexcept :
	m_yuv{std::move(src.m_yuv)}, m_size{src.m_size},
	m_subsample{src.m_subsample}, m_colorspace{src.m_colorspace}
{
	src.m_size.assign(0,0);
}

JPEG& JPEG::operator=(JPEG&& src) noexcept
{
	auto tmp{std::move(src)};
	m_yuv.swap(tmp.m_yuv);
	m_size = tmp.m_size;
	m_subsample  = tmp.m_subsample;
	m_colorspace = tmp.m_colorspace;
	return *this;
}

} // end of namespace hrb
