/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 11/26/18.
//

#include "FS.hh"

namespace hrb {

fs::path absolute(const fs::path& p, const fs::path& base)
{
	// copy from boost::absolute()
	if (p.has_root_directory())
		return p.has_root_name() ? p : absolute(base).root_name() / p;

	else
		return p.root_name() / absolute(base).root_directory() / absolute(base).relative_path() / p.relative_path();
}

}
