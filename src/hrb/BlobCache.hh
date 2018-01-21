/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/21/18.
//

#pragma once

#include <boost/beast/core/span.hpp>
#include <boost/filesystem/path.hpp>

namespace hrb {

class BlobCache
{
public:
	BlobCache(const boost::filesystem::path& cache_dir);


private:
	boost::filesystem::path m_cache_dir;
};

} // end of namespace
