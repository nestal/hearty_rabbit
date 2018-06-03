/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/15/18.
//

#include "BlobRequest.hh"
#include "crypto/Authentication.hh"

namespace hrb {

bool BlobRequest::request_by_owner(const Authentication& requester) const
{
	return !requester.is_guest() && requester.user() == m_url.user();
}

} // end of namespace