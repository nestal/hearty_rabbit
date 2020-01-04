/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/1/2020.
//

#include "client/HRBClient.hh"
#include "client/HRBClient.ipp"

#include "common/util/MMap.hh"
#include "common/util/Magic.hh"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

#include <iostream>
#include <fstream>

using namespace hrb;
using namespace std::chrono_literals;

Collection load_local(std::filesystem::path local)
{
	Magic magic;

	Collection coll;
	for (auto&& file : std::filesystem::directory_iterator{local})
	{
		std::cout << file.path() << std::endl;

		std::error_code ec;
		auto mmap = MMap::open(file.path(), ec);
		if (!ec)
		{
			Blake2 hash;
			hash.update(mmap.data(), mmap.size());

			coll.add_blob(
				hash.finalize(),
				BlobInode{
					{}, file.path().filename(), std::string{magic.mime(mmap.buffer())}, {}
				}
			);
		}
	}

	return coll;
}

int main(int argc, char **argv)
{
	if (argc < 6)
	{
		std::cerr << "usage: " << argv[0] << " [server] [port] [collection] [username] [password]" << std::endl;
		return -1;
	}

	load_local(std::filesystem::current_path());
	return 0;

	boost::asio::io_context ioc;
	ssl::context ctx{ssl::context::sslv23_client};

	HRBClient client{ioc, ctx, argv[1], argv[2]};
	client.login(argv[4], argv[5], [&client, coll=argv[3]](std::error_code ec)
	{
		if (!ec)
			std::cout << "login success!" << std::endl;

		client.list_collection(coll, [&client](Collection&& coll, std::error_code ec)
		{
			if (!ec)
				for (auto&& [id, entry] : coll.blobs())
				{
					std::cout << "blob: " << to_hex(id) << " " << entry.filename << std::endl;

					client.get_blob(coll.owner(), coll.name(), id, [fname=entry.filename](const std::string& body, std::error_code ec)
					{
						std::ofstream out{fname};
						out.rdbuf()->sputn(body.data(), body.size());
					});
				}
		});
	});
	ioc.run();

	return 0;
}
