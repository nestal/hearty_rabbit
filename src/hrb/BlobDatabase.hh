/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/17/18.
//

#pragma once

#include "ObjectID.hh"
#include "util/FS.hh"

#include <boost/beast/core/file_posix.hpp>

namespace hrb {

class ObjectID;

class BlobDatabase
{
public:
	class File
	{
	public:
		File() = default;

		/// Returns `true` if the file is open
        bool is_open() const;

        /// Close the file if open
        void close(boost::system::error_code& ec);

        /// Open a file at the given path with the specified mode
        void open(char const* path, boost::beast::file_mode mode, boost::system::error_code& ec);

        /// Return the size of the open file
        std::uint64_t size(boost::system::error_code& ec) const;

        /// Return the current position in the open file
        std::uint64_t pos(boost::system::error_code& ec) const;

        /// Adjust the current position in the open file
        void seek(std::uint64_t offset, boost::system::error_code& ec);

        /// Read from the open file
        std::size_t read(void* buffer, std::size_t n, boost::system::error_code& ec) const;

        /// Write to the open file
        std::size_t write(void const* buffer, std::size_t n, boost::system::error_code& ec);

		ObjectID ID();

	private:
		boost::beast::file_posix m_file;
		Blake2  m_hash;
	};

public:
	explicit BlobDatabase(const fs::path& base);

	File tmp_file() const;

private:
	fs::path    m_base;
};

} // end of namespace hrb
