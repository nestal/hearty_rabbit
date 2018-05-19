/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/28/18.
//

#pragma once

#include "util/Size2D.hh"
#include "util/BufferView.hh"

#include <opencv2/core.hpp>
#include <exception>
#include <string_view>
#include <vector>

namespace hrb {

cv::Mat load_image(BufferView raw);

class TurboBuffer;

class JPEG
{
public:
	class Exception : public std::exception
	{
	public:
		explicit Exception(const char *msg) : m_msg{msg} {}
		const char* what() const noexcept override {return m_msg;}

	private:
		const char *m_msg{};
	};

public:
	JPEG(const void *jpeg_data, std::size_t jpeg_size, const Size2D& max_dim);
	JPEG(JPEG&&) noexcept;
	JPEG(const JPEG&) = default;
	~JPEG() = default;

	JPEG& operator=(JPEG&&) noexcept;
	JPEG& operator=(const JPEG&) = default;

	Size2D size() const {return m_size;}
	TurboBuffer compress(int quality) const;

private:
	static Size2D select_scaling_factor(const Size2D& max, const Size2D& actual);

private:
	std::vector<unsigned char> m_yuv;

	Size2D m_size;
	int m_subsample{};
	int m_colorspace{};
};

} // end of namespace hrb
