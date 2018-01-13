/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/13/18.
//

#pragma once

#include <array>
#include <iosfwd>

namespace hrb {

class Backtrace
{
public:
	Backtrace();

	void update();

	friend std::ostream& operator<<(std::ostream& os, const Backtrace& bs);

private:
	std::array<void*, 100> m_stack;
	int m_count{0};
};

} // end of namespace
