/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#include "Container.hh"

#include <rapidjson/document.h>
#include <rapidjson/pointer.h>

namespace hrb {

const std::string_view Container::redis_prefix = "dir:";

Container::Container(std::string_view user, std::string_view path) :
	m_user{user},
	m_path{path}
{
}

} // end of namespace hrb
