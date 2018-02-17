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
#include <iostream>

namespace hrb {

BlobDatabase::BlobDatabase(const fs::path& base) : m_base{base}
{
	if (exists(base) && !is_directory(base))
		throw std::system_error(std::make_error_code(std::errc::file_exists));

	if (!exists(base))
		create_directories(base);
}

BlobDatabase::TempFile BlobDatabase::tmp_file() const
{
	boost::system::error_code err;

	TempFile result;
	result.open(m_base.string().c_str(), {}, err);
	if (err)
		throw std::system_error(err);
	return result;
}

fs::path BlobDatabase::save(BlobDatabase::TempFile&& tmp, std::error_code& ec)
{
	// move the file in argument to local variable to make sure the destructor
	// to delete the temp file.
	TempFile file{std::move(tmp)};
	auto dest_path = dest(file.ID());

	boost::system::error_code bec;
	if (!exists(dest_path.parent_path()))
		create_directories(dest_path.parent_path(), bec);
	ec.assign(bec.value(), bec.category());

	if (!ec)
	{
		std::ostringstream proc;
		proc << "/proc/self/fd/" << file.native_handle();

		if (::linkat(AT_FDCWD, proc.str().c_str(), AT_FDCWD, dest_path.string().c_str(), AT_SYMLINK_FOLLOW) != 0)
			ec.assign(errno, std::generic_category());
	}

	return dest_path;
}

fs::path BlobDatabase::dest(ObjectID id) const
{
	auto hex = to_hex(id);
	assert(hex.size() > 2);

	return m_base / hex.substr(0, 2) / hex;
}

bool BlobDatabase::TempFile::is_open() const
{
	return m_file.is_open();
}

void BlobDatabase::TempFile::close(boost::system::error_code& ec)
{
	return m_file.close(ec);
}

void BlobDatabase::TempFile::open(char const *path, boost::beast::file_mode, boost::system::error_code& ec)
{
	auto glibc_mkstemp = [path, this]
	{
		m_path = (fs::path{path} / "blob-XXXXXX").string();
		return ::mkstemp(&m_path[0]);
	};

#ifdef O_TMPFILE
//#if false
	// Note that O_TMPFILE requires the "path" to be a directory.
	// See http://man7.org/linux/man-pages/man2/open.2.html
	auto fd = ::open(path, O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);

	// Fallback to glibc mkstemp() if O_TMPFILE is not supported
	// by the Linux kernel.
	if (fd < 0 && errno == EISDIR)
		fd = glibc_mkstemp();
#else
	// CentOS 7 (i.e. Linux 3.10) does not support O_TMPFILE, so
	// don't bother even trying.
	auto fd = glibc_mkstemp();
#endif

	if (fd < 0)
		ec.assign(errno, boost::system::generic_category());
	else
		m_file.native_handle(fd);
}

std::uint64_t BlobDatabase::TempFile::size(boost::system::error_code& ec) const
{
	return m_file.size(ec);
}

std::uint64_t BlobDatabase::TempFile::pos(boost::system::error_code& ec) const
{
	return m_file.pos(ec);
}

void BlobDatabase::TempFile::seek(std::uint64_t offset, boost::system::error_code& ec)
{
	return m_file.seek(offset, ec);
}

std::size_t BlobDatabase::TempFile::read(void *buffer, std::size_t n, boost::system::error_code& ec) const
{
	return m_file.read(buffer, n, ec);
}

std::size_t BlobDatabase::TempFile::write(void const *buffer, std::size_t n, boost::system::error_code& ec)
{
	m_hash.update(buffer, n);
	return m_file.write(buffer, n, ec);
}

/// Get the object ID (blake2 hash) of the file.
ObjectID BlobDatabase::TempFile::ID() const
{
	return ObjectID{Blake2{m_hash}.finalize()};
}

boost::beast::file_posix::native_handle_type BlobDatabase::TempFile::native_handle() const
{
	return m_file.native_handle();
}

BlobDatabase::TempFile::~TempFile()
{
	if (!m_path.empty())
		fs::remove(m_path);
}

} // end of namespace hrb
