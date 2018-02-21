/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/17/18.
//

#include "BlobDatabase.hh"
#include "BlobObject.hh"
#include "UploadFile.hh"

#include <sstream>
#include <iostream>

namespace hrb {

BlobDatabase::BlobDatabase(const fs::path& base) : m_base{base}
{
	if (exists(base) && !is_directory(base))
		throw std::system_error(std::make_error_code(std::errc::file_exists));

	if (!exists(base))
		create_directories(base);
}

void BlobDatabase::prepare_upload(UploadFile& result, std::error_code& ec) const
{
	boost::system::error_code err;

	result.open(m_base.string().c_str(), err);
	if (err)
		throw std::system_error(err);
}

fs::path BlobDatabase::save(UploadFile&& tmp, std::error_code& ec)
{
	// move the file in argument to local variable to make sure the destructor
	// to delete the temp file.
	auto file{std::move(tmp)};
	auto dest_path = dest(file.ID());
	static_assert(std::is_same<decltype(file), UploadFile>::value);

	boost::system::error_code bec;
	if (!exists(dest_path.parent_path()))
		create_directories(dest_path.parent_path(), bec);
	ec.assign(bec.value(), bec.category());

	if (!ec)
		file.linkat(dest_path, bec);

	return dest_path;
}

fs::path BlobDatabase::dest(ObjectID id) const
{
	auto hex = to_hex(id);
	assert(hex.size() > 2);

	return m_base / hex.substr(0, 2) / hex;
}

} // end of namespace hrb
