/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/2/18.
//

#pragma once

#include <ostream>

namespace hrb {

template <typename T>
class BasicSize2D
{
public:
	BasicSize2D() = default;
	BasicSize2D(T width, T height) : m_width{width}, m_height{height} {}

	constexpr T width() const {return m_width;}
	constexpr T height() const {return m_height;}
	T& width() {return m_width;}
	T& height() {return m_height;}

	void width(T w) {m_width = w;}
	void height(T h) {m_height = h;}
	void assign(T w, T h) {m_width = w; m_height = h;}
	constexpr T area() const {return m_width * m_height;}

	bool operator==(const BasicSize2D& other) const {return m_width == other.m_width && m_height == other.m_height;}
	bool operator!=(const BasicSize2D& other) const {return m_width != other.m_width || m_height != other.m_height;}

private:
	T m_width{};
	T m_height{};
};

using Size2D = BasicSize2D<int>;

template <typename T>
std::ostream& operator<<(std::ostream& os, const BasicSize2D<T>& size)
{
	return os << R"({"width":)" << size.width() << R"(, "height": )" << size.height() << "}";
}

} // end of namespace hrb
