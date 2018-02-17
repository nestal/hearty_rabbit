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

BlobDatabase::File BlobDatabase::tmp_file() const
{
	boost::system::error_code err;

	File result;
	result.open(m_base.string().c_str(), {}, err);
	if (err)
		throw std::system_error(err);
	return result;
}

fs::path BlobDatabase::save(BlobDatabase::File&& tmp, std::error_code& ec)
{
	File file{std::move(tmp)};
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

#ifndef O_TMPFILE
		auto tmp_file = fs::read_symlink(proc.str(), bec);
		std::cout << "readlink: " << bec.message() << " " << tmp_file << std::endl;
		remove(tmp_file);
#endif
	}

	return dest_path;
}

fs::path BlobDatabase::dest(ObjectID id) const
{
	auto hex = to_hex(id);
	assert(hex.size() > 2);

	return m_base / hex.substr(0, 2) / hex;
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
	auto glibc_mkstemp = [path]
	{
		auto pathstr = (fs::path{path} / "blob-XXXXXX").string();
		return ::mkstemp(&pathstr[0]);
	};

#ifdef O_TMPFILE
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

/// Get the object ID (blake2 hash) of the file.
/// This function is not const. It will consume the internal hash object such that subsequent
/// calls to write() cannot update it. Do not call write() after calling this function.
ObjectID BlobDatabase::File::ID()
{
	return ObjectID{m_hash.finalize()};
}

boost::beast::file_posix::native_handle_type BlobDatabase::File::native_handle() const
{
	return m_file.native_handle();
}

} // end of namespace hrb
