/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 5/1/2020.
//

#pragma once

#include "hrb/Collection.hh"

namespace hrb {

class CollectionComparison
{
public:
	CollectionComparison(const Collection& local, const Collection& remote);

	auto& upload() const {return m_upload;}
	auto& download() const {return m_download;}

private:
	Collection m_upload, m_download;
};

} // end of namespace hrb
