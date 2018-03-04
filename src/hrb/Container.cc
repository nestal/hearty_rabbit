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

namespace hrb {

const std::string_view Container::redis_prefix = "dir:";

std::string Entry::JSON(const ObjectID& blob, std::string_view mime)
{
	// too simple to bother the json library
	std::ostringstream ss;
	ss  << "{"
		<< R"("blob": ")" << to_hex(blob) << "\","
		<< R"("mime": ")" << mime         << '\"'
		<< "}";
	return ss.str();
}

std::string Entry::JSON() const
{
	return JSON(m_blob, m_mime);
}

Entry::Entry(std::string_view filename, std::string_view json)
{

}

} // end of namespace hrb
