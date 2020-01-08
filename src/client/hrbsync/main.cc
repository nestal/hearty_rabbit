/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/1/2020.
//

#include "CollectionComparison.hh"

#include "http/HRBClient.hh"
#include "http/HRBClient.ipp"

#include "util/MMap.hh"
#include "image/Image.hh"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

#include <iostream>
#include <fstream>

using namespace hrb;
using namespace std::chrono_literals;

void download_difference(const CollectionComparison& comp, HRBClient& client)
{
	auto& download = comp.download();
	std::cout << "downloading " << download.size() << " files" << std::endl;

	client.download_collection(download, "master", std::filesystem::current_path(),
		[](std::error_code ec)
		{
			std::cout << "downloaded finished!" << ec << std::endl;
		}
	);
}

int main(int argc, char **argv)
{
	if (argc < 6)
	{
		std::cerr << "usage: " << argv[0] << " [server] [port] [collection] [username] [password]" << std::endl;
		return -1;
	}

	Collection local{std::filesystem::current_path()};

	boost::asio::io_context ioc;
	ssl::context ctx{ssl::context::sslv23_client};

	HRBClient client{ioc, ctx, argv[1], argv[2]};
	client.login(argv[4], argv[5], [&client, coll=argv[3], &local](std::error_code ec)
	{
		if (!ec)
			std::cout << "login success!" << std::endl;

		client.list_collection(coll, [&client, &local](Collection&& coll, std::error_code ec)
		{
			if (!ec)
				download_difference(CollectionComparison{local, coll}, client);
		});
	});
	ioc.run();

	return 0;
}
