/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 9/11/2021.
//

#pragma once

namespace hrb {

// Helper type for the std::variant visitor
// See https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct Overloaded : Ts...
{
	using Ts::operator()...;
};

// explicit deduction guide (not needed as of C++20)
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

} // end of namespace
