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

#include <sstream>

namespace hrb {

BlobDatabase::BlobDatabase(const fs::path& base) : m_base{base}
{

}

BlobDatabase::File BlobDatabase::tmp_file() const
{
	boost::system::error_code err;

	File result;
	result.open(m_base.string().c_str(), {}, err);
	if (err)
		throw std::system_error(err);
	return result;
}

fs::path BlobDatabase::open(BlobDatabase::File&& tmp) const
{
	File file{std::move(tmp)};
	auto dest = m_base/to_hex(file.ID());

	std::ostringstream proc;
	proc << "/proc/self/fd/" << file.native_handle();
	::linkat(AT_FDCWD, proc.str().c_str(), AT_FDCWD, dest.string().c_str(), AT_SYMLINK_FOLLOW);

	return dest;
}

bool BlobDatabase::File::is_open() const
{
	return m_file.is_open();
}

void BlobDatabase::File::close(boost::system::error_code& ec)
{
	return m_file.close(ec);
}

void BlobDatabase::File::open(char const *path, boost::beast::file_mode, boost::system::error_code& ec)
{
	// Note that O_TMPFILE requires the "path" to be a directory.
	// See http://man7.org/linux/man-pages/man2/open.2.html
	auto fd = ::open(path, __O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd < 0)
		ec.assign(errno, boost::system::generic_category());
	else
		m_file.native_handle(fd);
}

std::uint64_t BlobDatabase::File::size(boost::system::error_code& ec) const
{
	return m_file.size(ec);
}

std::uint64_t BlobDatabase::File::pos(boost::system::error_code& ec) const
{
	return m_file.pos(ec);
}

void BlobDatabase::File::seek(std::uint64_t offset, boost::system::error_code& ec)
{
	return m_file.seek(offset, ec);
}

std::size_t BlobDatabase::File::read(void *buffer, std::size_t n, boost::system::error_code& ec) const
{
	return m_file.read(buffer, n, ec);
}

std::size_t BlobDatabase::File::write(void const *buffer, std::size_t n, boost::system::error_code& ec)
{
	m_hash.update(buffer, n);
	return m_file.write(buffer, n, ec);
}

ObjectID BlobDatabase::File::ID()
{
	return ObjectID{m_hash.finalize()};
}

boost::beast::file_posix::native_handle_type BlobDatabase::File::native_handle() const
{
	return m_file.native_handle();
}

} // end of namespace hrb
