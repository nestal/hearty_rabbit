/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/3/18.
//

#include "BlobObject.hh"
#include "BlobMeta.hh"

namespace hrb {

BlobObject::BlobObject(const fs::path& base, const ObjectID& id) : m_base{base}, m_id{id}
{

}

BlobMeta BlobObject::meta() const
{
	return BlobMeta();
}
} // end of namespace hrb
