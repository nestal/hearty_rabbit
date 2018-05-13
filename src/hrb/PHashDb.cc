/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 5/13/18.
//

#include "PHashDb.hh"

namespace hrb {

PHashDb::PHashDb(redis::Connection& db) : m_db{db}
{
}

} // end of namespace hrb
