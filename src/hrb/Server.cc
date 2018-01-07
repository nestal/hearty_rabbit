/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/7/18.
//

#include "Server.hh"

namespace hrb {

Server::Server(const boost::filesystem::path &doc_root) :
	m_doc_root{doc_root}
{
}

} // end of namespace
