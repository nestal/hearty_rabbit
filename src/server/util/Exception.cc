/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/9/18.
//

#include "Exception.hh"

#include <boost/exception/diagnostic_information.hpp>

namespace hrb {

const char* Exception::what() const noexcept
{
	return boost::diagnostic_information_what(*this, true);
}

} // end of namespace
