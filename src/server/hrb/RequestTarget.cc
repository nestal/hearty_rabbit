/*
	Copyright © 2023 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 7/10/23.
//

#include "RequestTarget.hh"

namespace hrb {

RequestTarget::RequestTarget(std::string_view target)
{
	if (target.empty())
		m_path = "/";
}

} // end of namespace hrb