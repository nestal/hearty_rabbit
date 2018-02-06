/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/7/18.
//

#include "Server.hh"
#include "WebResources.hh"
#include "BlobObject.hh"

#include "crypto/Password.hh"
#include "crypto/Authenication.hh"
#include "net/Listener.hh"
#include "util/Configuration.hh"
#include "util/Exception.hh"
#include "util/Escape.hh"
#include "util/Log.hh"

#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/info.hpp>

#include <iostream>

namespace hrb {

Server::Server(const Configuration& cfg) :
	m_cfg{cfg},
	m_ioc{static_cast<int>(std::max(1UL, cfg.thread_count()))},
	m_db{cfg.redis()}
{
	OpenSSL_add_all_digests();
}

void Server::on_login(const Request& req, EmptyResponseSender&& send)
{
	auto&& body = req.body();
	if (req[http::field::content_type] == "application/x-www-form-urlencoded")
	{
		auto [username, password] = find_fields(body, "username", "password");

		auto db = m_db.alloc(m_ioc);
		verify_user(
			username,
			Password{password},
			*db,
			[
				db,
				version=req.version(),
				send=std::move(send),
				this,
				keep_alive=req.keep_alive()
			](std::error_code ec, auto&& session) mutable
			{
				Log(LOG_INFO, "login result: %1% %2%", ec, ec.message());
				m_db.release(std::move(db));

				auto&& res = redirect(ec ? "/login_incorrect.html" : "/index.html", version);
				res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
				res.keep_alive(keep_alive);

				if (!ec)
					res.insert(http::field::set_cookie, set_cookie(session));

				send(std::move(res));
			}
		);
	}
	else
		send(set_common_fields(req, redirect("/login.html", req.version())));
}

void Server::on_logout(const Request& req, const SessionID& id, EmptyResponseSender&& send)
{
	auto db = m_db.alloc(m_ioc);
	destroy_session(id, *db, [this, db, send=std::move(send), version=req.version(), keep_alive=req.keep_alive()](auto&& ec) mutable
	{
		m_db.release(std::move(db));

		auto&& res = redirect("/login.html", version);
		res.insert(http::field::set_cookie, "id=; ");
		send(set_common_fields(keep_alive, std::move(res)));
	});
}

http::response<http::empty_body> Server::redirect(boost::beast::string_view where, unsigned version)
{
	http::response<http::empty_body> res{http::status::moved_permanently, version};
	res.set(http::field::location, where);
	return res;
}

void Server::get_blob(const Request& req, StringResponseSender&& send)
{
	auto blob_id = req.target().size() > url::login.size() ?
		req.target().substr(url::login.size()) :
		boost::string_view{};

	auto object_id = hex_to_object_id(std::string_view{blob_id.data(), blob_id.size()});
	if (object_id == ObjectID{})
		send(set_common_fields(req, not_found(req, to_hex(object_id))));
	else
	{
		auto db = m_db.alloc(m_ioc);
		BlobObject::load(
			*db, object_id,
			[db, this, send=std::move(send), version=req.version(), keep_alive=req.keep_alive()](BlobObject& blob, std::error_code ec) mutable
			{
				m_db.release(std::move(db));

				http::response<http::string_body> res{!ec ? http::status::ok : http::status::not_found, version};
				res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
				res.set(http::field::content_type, blob.mime());
				res.keep_alive(keep_alive);
				if (!ec)
					res.body() = blob.blob();
				send(std::move(res));
			}
		);
	}
}

void Server::on_upload(const Request& req, StringResponseSender&& send)
{
	std::cout << "method = " << req.method() << " size = " << req.at(http::field::content_length) << std::endl;
//	std::cout << "content = \n" << req.body() << std::endl;

	for (auto&& field : req)
		std::cout << field.name() << " " << field.value() << std::endl;

	http::response<http::string_body> res{http::status::ok, req.version()};
	res.body() = "OK";
	res.insert(http::field::content_type, "text/plain");
	send(set_common_fields(req, std::move(res)));
}

void Server::on_invalid_session(const Request& req, std::function<void(http::response<http::empty_body>&&)>&& send)
{
	// Introduce a small delay when responsing to requests with invalid session ID.
	// This is to slow down bruce-force attacks on the session ID.
	boost::asio::deadline_timer t{m_ioc, boost::posix_time::milliseconds{500}};
	return t.async_wait([version=req.version(), send=std::move(send)](auto ec)
	{
		send(redirect("/login.html", version));
	});
}

http::response<http::string_body> Server::get_dir(const Request& req)
{
	return http::response<http::string_body>();
}

http::response<http::string_body> Server::bad_request(const Request& req, boost::beast::string_view why)
{
	http::response<http::string_body> res{http::status::bad_request, req.version()};
	res.set(http::field::content_type, "text/html");
	res.body() = why.to_string();
	res.prepare_payload();
	return set_common_fields(req, std::move(res));
}

// Returns a not found response
http::response<http::string_body> Server::not_found(const Request& req, boost::beast::string_view target)
{
	http::response<http::string_body> res{http::status::not_found, req.version()};
	res.set(http::field::content_type, "text/html");
	res.body() = "The resource '" + target.to_string() + "' was not found.";
	res.prepare_payload();
	return set_common_fields(req, std::move(res));
}

http::response<http::string_body> Server::server_error(const Request& req, boost::beast::string_view what)
{
	http::response<http::string_body> res{http::status::internal_server_error, req.version()};
	res.set(http::field::content_type, "text/html");
	res.body() = "An error occurred: '" + what.to_string() + "'";
	res.prepare_payload();
	return set_common_fields(req, std::move(res));
}

std::optional<http::response<http::file_body>> Server::file_request(const Request& req)
{
	Log(LOG_NOTICE, "requesting path %1%", req.target());

	auto filepath = req.target();
	filepath.remove_prefix(1);

	// TODO: use redirect instead
	if (web_resources.find(filepath.to_string()) == web_resources.end())
		return std::nullopt;

	auto path = m_cfg.web_root() / filepath.to_string();
	Log(LOG_NOTICE, "reading from %1%", path);

	// Attempt to open the file
	boost::beast::error_code ec;
	http::file_body::value_type file;
	file.open(path.string().c_str(), boost::beast::file_mode::scan, ec);

	// Handle the case where the file doesn't exist
	if (ec)
		throw std::system_error(ec);

	auto file_size = file.size();

	http::response<http::file_body> res{
		std::piecewise_construct,
		std::make_tuple(std::move(file)),
		std::make_tuple(http::status::ok, req.version())
	};
	res.set(http::field::content_type, resource_mime(path.extension().string()));
	res.content_length(file_size);
	return set_common_fields(req, std::move(res));
}

http::response<http::empty_body> Server::redirect_http(const Request &req)
{
	using namespace std::literals;
	static const auto https_host = "https://" + m_cfg.server_name()
		+ (m_cfg.listen_https().port() == 443 ? ""s : (":"s + std::to_string(m_cfg.listen_https().port())));

	auto&& dest = https_host + req.target().to_string();
	Log(LOG_INFO, "redirecting HTTP request %1% to host %2%", req.target(), dest);

	return set_common_fields(req, redirect(dest, req.version()));
}

std::string_view Server::resource_mime(const std::string& ext)
{
	// don't expect a big list
	     if (ext == ".html")    return "text/html";
	else if (ext == ".css")     return "text/css";
	else if (ext == ".svg")     return "image/svg+xml";
	else                        return "application/octet-stream";
}

void Server::run()
{
	auto const threads = std::max(1UL, m_cfg.thread_count());

	boost::asio::ssl::context ctx{boost::asio::ssl::context::sslv23};
	ctx.set_options(
		boost::asio::ssl::context::default_workarounds |
		boost::asio::ssl::context::no_sslv2
	);
	ctx.use_certificate_chain_file(m_cfg.cert_chain().string());
	ctx.use_private_key_file(m_cfg.private_key().string(), boost::asio::ssl::context::pem);

	// Create and launch a listening port for HTTP and HTTPS
	std::make_shared<Listener>(
		m_ioc,
		m_cfg.listen_http(),
		*this,
		nullptr
	)->run();
	std::make_shared<Listener>(
		m_ioc,
		m_cfg.listen_https(),
		*this,
		&ctx
	)->run();

	// make sure we load the certificates and listen before dropping root privileges
	drop_privileges();

	// Run the I/O service on the requested number of threads
	std::vector<std::thread> v;
	v.reserve(threads - 1);
	for (auto i = threads - 1; i > 0; --i)
		v.emplace_back([this]{m_ioc.run();});

	m_ioc.run();
}

void Server::drop_privileges()
{
	// drop privileges if run as root
	if (::getuid() == 0)
	{
		if (::setuid(65535) != 0)
			BOOST_THROW_EXCEPTION(hrb::SystemError()
				<< ErrorCode(std::error_code(errno, std::system_category()))
				<< boost::errinfo_api_function("setuid")
			);
	}

	if (::getuid() == 0)
		throw std::runtime_error("cannot run as root");
}

void Server::add_user(std::string_view username, Password&& password, std::function<void(std::error_code)> complete)
{
	auto db = m_db.alloc(m_ioc);
	hrb::add_user(username, std::move(password), *db, [db, &complete](std::error_code&& ec)
	{
		complete(std::move(ec));
		db->disconnect();
	});
	m_ioc.run();
}

void Server::add_blob(const boost::filesystem::path& path, std::function<void(BlobObject&, std::error_code)> complete)
{
	auto db = m_db.alloc(m_ioc);
	BlobObject blob{path};
	blob.save(*db, [db, complete=std::move(complete)](BlobObject& blob, std::error_code&& ec)
	{
		complete(blob, ec);
		db->disconnect();
	});
	m_ioc.run();
}

boost::asio::io_context& Server::get_io_context()
{
	return m_ioc;
}

void Server::disconnect_db()
{
	m_db.release_all();
}

bool Server::allow_anonymous(boost::string_view target)
{
	assert(!target.empty());
	assert(target.front() == '/');
	target.remove_prefix(1);

	return target != "index.html" && web_resources.find(target.to_string()) != web_resources.end();
}

} // end of namespace
