/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/24/18.
//

#include "MMapResponseBody.hh"

namespace hrb {

std::uint64_t MMapResponseBody::size(const value_type& body)
{
    return body.size();
}

void MMapResponseBody::writer::init(boost::system::error_code& ec)
{
    ec.assign(0, ec.category());
}

boost::optional<std::pair<MMapResponseBody::writer::const_buffers_type, bool>>
MMapResponseBody::writer::get(boost::system::error_code& ec)
{
    ec.assign(0, ec.category());

    return {
        {m_body.blob(), false} // pair
    }; // optional
}

} // end of namespace hrb
