/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#include "GenericHTTPRequest.hh"

#include <iostream>

namespace hrb {

// Report a failure
void fail(boost::system::error_code ec, char const *what)
{
	std::cerr << what << ": " << ec.message() << "\n";
}

}
