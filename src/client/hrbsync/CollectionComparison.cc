/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 5/1/2020.
//

#include "CollectionComparison.hh"

#include <vector>
#include <algorithm>

namespace hrb {

CollectionComparison::CollectionComparison(const Collection& local, const Collection& remote)
{
	std::set<ObjectID> remote_blobids, local_blobids;
	for (auto&& [id, blob] : remote)
		remote_blobids.insert(id);

	for (auto&& [id, blob] : local)
		local_blobids.insert(id);

	std::vector<ObjectID> download;
	std::set_difference(
		remote_blobids.begin(), remote_blobids.end(),
		local_blobids.begin(), local_blobids.end(),
		std::back_inserter(download)
	);
	for (auto&& id : download)
		m_download.add_blob(id, remote.find(id)->second);

	std::vector<ObjectID> upload;
	std::set_difference(
		local_blobids.begin(), local_blobids.end(),
		remote_blobids.begin(), remote_blobids.end(),
		std::back_inserter(upload)
	);
	for (auto&& id : upload)
		m_upload.add_blob(id, local.find(id)->second);
}

} // end of namespace hrb
