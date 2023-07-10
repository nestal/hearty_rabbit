/*
	Copyright © 2023 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 7/10/23.
//

#pragma once

#include <string_view>

namespace hrb {

class RequestTarget
{
public:
	explicit RequestTarget(std::string_view target = {});

	[[nodiscard]] auto path() const {return m_path;}
	[[nodiscard]] auto action() const {return m_action;}

private:
	std::string_view m_path;
	std::string_view m_action;
};

} // end of namespace
