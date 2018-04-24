/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

#include "BinStruct.hh"

namespace hrb {

void BinStruct::pack(const std::string& s)
{
	// null terminated
	m_bytes.insert(m_bytes.end(), s.begin(), s.end());
	m_bytes.push_back('\0');
}

} // end of namespace
