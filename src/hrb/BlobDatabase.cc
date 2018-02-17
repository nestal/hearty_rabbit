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

namespace hrb {

BlobDatabase::BlobDatabase(const fs::path& base) : m_base{base}
{

}

boost::beast::file_posix BlobDatabase::tmp_file() const
{
	boost::beast::file_posix result;
	auto fd = ::open(m_base.string().c_str(), O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd < 0)
		throw std::system_error(errno, std::generic_category());
	result.native_handle(fd);

	return result;
}

} // end of namespace hrb
