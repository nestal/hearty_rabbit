/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/21/18.
//

#include "BlobCache.hh"

namespace hrb {

BlobCache::BlobCache(const boost::filesystem::path& cache_dir) :
	m_cache_dir{cache_dir}
{
}

} // end of namespace
