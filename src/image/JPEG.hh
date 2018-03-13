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

#include <string_view>
#include <exception>
#include <vector>

namespace hrb {

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
	JPEG(const void *data, std::size_t size, const Size2D& max_dim);
	JPEG(JPEG&&) = default;
	JPEG(const JPEG&) noexcept = default;
	~JPEG() = default;

	JPEG& operator=(JPEG&&) noexcept = default;
	JPEG& operator=(const JPEG&) = default;

	Size2D size() const {return m_size;}
	TurboBuffer compress(int quality) const;

private:
	static Size2D select_scaling_factor(const Size2D& max, const Size2D& actual);

private:
	std::vector<unsigned char> m_pixels;

	Size2D m_size;
	int m_subsample{};
	int m_colorspace{};
};

} // end of namespace hrb
