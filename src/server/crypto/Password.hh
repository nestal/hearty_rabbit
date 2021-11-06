/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/27/18.
//

#pragma once

#include "util/BufferView.hh"

#include <array>
#include <vector>

namespace hrb {

class Password
{
public:
	explicit Password(std::string_view val);
	~Password();

	Password() = default;
	Password(Password&& other) noexcept = default;
	Password(const Password&) = delete;

	Password& operator=(Password&& other) noexcept = default;
	Password& operator=(const Password&) = delete;
	void swap(Password& other);

	using Key = std::array<unsigned char, 64>;
	[[nodiscard]] Key derive_key(BufferView salt, std::size_t iteration, const std::string& hash_name) const;
	[[nodiscard]] std::string_view get() const;

	void clear();
	[[nodiscard]] bool empty() const;
	[[nodiscard]] std::size_t size() const;

private:
	std::vector<char>   m_val;
};

} // end of namespace hrb
