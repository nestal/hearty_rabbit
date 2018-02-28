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

#include <turbojpeg.h>

#include <memory>

namespace hrb {

JPEG::JPEG(const void *data, std::size_t size, int max_width, int max_height)
{
	std::unique_ptr<void, decltype(&tjDestroy)> handle{tjInitDecompress(), &tjDestroy};
	auto result = tjDecompressHeader3(
		handle.get(), static_cast<const unsigned char*>(data), size,
		&m_width, &m_height, &m_subsample, &m_colorspace
	);

	if (result != 0)
		throw JPEGException(tjGetErrorStr());

	int width = m_width, height = m_height;
	select_scaling_factor(m_width, max_height, width, height);

	// allocate pixels
	auto pixel_size = height * width * tjPixelSize[TJPF_RGB];
	std::vector<unsigned char> pixels(pixel_size);

	result = tjDecompress2(
		handle.get(), static_cast<const unsigned char*>(data), size,
		&pixels[0], width, 0, height, TJPF_RGB, TJFLAG_FASTDCT
	);
	if (result != 0)
		throw JPEGException(tjGetErrorStr());

	// commit result
	m_pixels = std::move(pixels);
	m_width = width;
	m_height = height;
}

void JPEG::select_scaling_factor(int max_width, int max_height, int& width, int& height)
{
	if (width > max_width || height > max_height)
	{
		width  = 0;
		height = 0;

		int count{};
		auto factors = tjGetScalingFactors(&count);
		for (int i = 0; i < count; i++)
		{
			auto w = TJSCALED(m_width,  factors[i]);
			auto h = TJSCALED(m_height, factors[i]);

			// width and height within limits, and total area larger
			// than precious accepted values
			if (w < max_width && h < max_height && w*h > width * height)
			{
				// remember this scaling factor
				width = w;
				height = h;
			}
		}
	}
}

} // end of namespace hrb
