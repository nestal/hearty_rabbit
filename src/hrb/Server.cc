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
#include <boost/filesystem.hpp>

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
				this
			](std::error_code ec, auto&& session) mutable
			{
				Log(LOG_INFO, "login result: %1% %2%", ec, ec.message());
				m_db.release(std::move(db));

				auto&& res = redirect(ec ? "/login_incorrect.html" : "/", version);
				if (!ec)
					res.set(http::field::set_cookie, set_cookie(session));

				send(std::move(res));
			}
		);
	}
	else
		send(redirect("/login.html", req.version()));
}

void Server::on_logout(const Request& req, const SessionID& id, EmptyResponseSender&& send)
{
	auto db = m_db.alloc(m_ioc);
	destroy_session(id, *db, [this, db, send=std::move(send), version=req.version()](auto&& ec) mutable
	{
		m_db.release(std::move(db));

		auto&& res = redirect("/login.html", version);
		res.set(http::field::set_cookie, "id=; ");
		res.keep_alive(false);
		send(std::move(res));
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
		send(not_found(req));
	else
	{
		auto db = m_db.alloc(m_ioc);
		BlobObject::load(
			*db, object_id,
			[db, this, send=std::move(send), version=req.version()](BlobObject& blob, std::error_code ec) mutable
			{
				m_db.release(std::move(db));

				http::response<http::string_body> res{
					std::piecewise_construct,
					std::make_tuple(ec ? std::string_view{} : blob.string()),
					std::make_tuple(ec ? http::status::not_found : http::status::ok, version)
				};
				res.set(http::field::content_type, blob.mime());
				res.prepare_payload();
				send(std::move(res));
			}
		);
	}
}

void Server::on_upload(Request&& req, EmptyResponseSender&& send)
{
//	std::cout << "method = " << req.method() << " size = " << req.at(http::field::content_length) << std::endl;

	auto [prefix, filename] = extract_prefix(req);
//	std::cout << "file = " << filename << std::endl;

//	for (auto&& field : req)
//		std::cout << field.name() << " " << field.value() << std::endl;

	BlobObject blob{std::move(req).body(), filename};
	Log(LOG_INFO, "uploading %1% bytes to %2%", blob.size(), filename);

	auto db = m_db.alloc(m_ioc);
	blob.save(*db, [this, db, send=std::move(send), version=req.version()](BlobObject& blob, auto ec) mutable
	{
		m_db.release(std::move(db));

		http::response<http::empty_body> res{http::status::created, version};
		res.set(http::field::location, "/blob/" + to_hex(blob.ID()));
		return send(std::move(res));
	});
}

void Server::on_invalid_session(const Request& req, EmptyResponseSender&& send)
{
	bool target_home = (req.target() == "/");

	// Introduce a small delay when responsing to requests with invalid session ID.
	// This is to slow down bruce-force attacks on the session ID.
	boost::asio::deadline_timer t{m_ioc, boost::posix_time::milliseconds{500}};
	return t.async_wait([version=req.version(), target_home, send=std::move(send)](auto ec)
	{
		if (!ec)
			Log(LOG_WARNING, "timer error %1% (%2%)", ec, ec.message());

		// If the target is home (i.e. "/"), redirect to login page.
		// Because it may be because the user's session just exprired.
		send(
			target_home ?
				redirect("/login.html", version) :
				http::response<http::empty_body>{http::status::forbidden, version}
		);
	});
}

http::response<http::string_body> Server::get_dir(const Request& req)
{
	return http::response<http::string_body>();
}

http::response<http::string_body> Server::bad_request(const Request& req, boost::beast::string_view why)
{
	http::response<http::string_body> res{
		std::piecewise_construct,
		std::make_tuple(why),
		std::make_tuple(http::status::bad_request, req.version())
	};
	res.set(http::field::content_type, "text/html");
	res.prepare_payload();
	return res;
}

// Returns a not found response
http::response<http::string_body> Server::not_found(const Request& req)
{
	using namespace std::literals;
	http::response<http::string_body> res{
		std::piecewise_construct,
		std::make_tuple("The resource '"s + req.target().to_string() + "' was not found."),
		std::make_tuple(http::status::not_found, req.version())
	};
	res.set(http::field::content_type, "text/plain");
	res.prepare_payload();
	return res;
}

http::response<http::string_body> Server::server_error(const Request& req, boost::beast::string_view what)
{
	http::response<http::string_body> res{
		std::piecewise_construct,
		std::make_tuple("An error occurred: '" + what.to_string() + "'"),
		std::make_tuple(http::status::internal_server_error, req.version())
	};
	res.set(http::field::content_type, "text/plain");
	res.prepare_payload();
	return res;
}

http::response<http::file_body> Server::serve_home(unsigned version)
{
	return file_request(m_cfg.web_root() / "dynamic" / "index.html", version);
}

http::response<http::file_body> Server::static_file_request(const Request& req)
{
	Log(LOG_NOTICE, "requesting path %1%", req.target());

	auto filepath = req.target();
	filepath.remove_prefix(1);

	return file_request(boost::filesystem::path{"static"} / filepath.to_string(), req.version());
}

http::response<http::file_body> Server::file_request(const boost::filesystem::path& req_path, unsigned version)
{
	auto path = canonical(req_path, m_cfg.web_root());
	Log(LOG_NOTICE, "reading from %1%", path);

	// Attempt to open the file
	boost::beast::error_code ec;
	http::file_body::value_type file;
	file.open(path.string().c_str(), boost::beast::file_mode::scan, ec);

	// Handle the case where the file doesn't exist
	if (ec)
	{
		file.open(
			(m_cfg.web_root() / "dynamic" / "not_found.html").string().c_str(),
			boost::beast::file_mode::scan,
			ec
		);
		if (ec)
			throw std::system_error(ec);
	}

	auto file_size = file.size();

	http::response<http::file_body> res{
		std::piecewise_construct,
		std::make_tuple(std::move(file)),
		std::make_tuple(http::status::ok, version)
	};
	res.set(http::field::content_type, resource_mime(path.extension().string()));
	res.content_length(file_size);
	return std::move(res);
}

http::response<http::empty_body> Server::redirect_http(const Request &req)
{
	using namespace std::literals;
	static const auto https_host = "https://" + m_cfg.server_name()
		+ (m_cfg.listen_https().port() == 443 ? ""s : (":"s + std::to_string(m_cfg.listen_https().port())));

	auto&& dest = https_host + req.target().to_string();
	Log(LOG_INFO, "redirecting HTTP request %1% to host %2%", req.target(), dest);

	return redirect(dest, req.version());
}

std::string_view Server::resource_mime(const std::string& ext)
{
	// don't expect a big list
	     if (ext == ".html")    return "text/html";
	else if (ext == ".css")     return "text/css";
	else if (ext == ".svg")     return "image/svg+xml";
    else if (ext == ".js")      return "application/javascript";
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

	return web_resources.find(target.to_string()) != web_resources.end();
}

std::tuple<
	std::string_view,
	std::string_view
> Server::extract_prefix(const Request& req)
{
	auto target = req.target();
	auto sv = std::string_view{target.data(), target.size()};
	if (!sv.empty() && sv.front() == '/')
		sv.remove_prefix(1);

	auto prefix = std::get<0>(split_front(sv, "/?$"));
	return std::make_tuple(prefix, sv);
}

} // end of namespace
