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

#include "Ownership.hh"
#include "Ownership.ipp"
#include "UploadFile.hh"
#include "BlobFile.hh"
#include "URLIntent.hh"
#include "BlobRequest.hh"

#include "crypto/Password.hh"
#include "crypto/Authentication.hh"

#include "net/Listener.hh"
#include "net/SplitBuffers.hh"
#include "net/MMapResponseBody.hh"

#include "util/Error.hh"
#include "util/Configuration.hh"
#include "util/Exception.hh"
#include "util/Log.hh"
#include "util/FS.hh"
#include "util/Escape.hh"
#include "util/JsonHelper.hh"

#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/info.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

namespace hrb {

namespace {
const std::string_view index_needle{"<meta charset=\"utf-8\"><script>"};
}

Server::Server(const Configuration& cfg) :
	m_cfg{cfg},
	m_ioc{static_cast<int>(std::max(1UL, cfg.thread_count()))},
	m_db{m_ioc, cfg.redis()},
	m_lib{cfg.web_root()},
	m_blob_db{cfg}
{
	OpenSSL_add_all_digests();
}

void Server::on_login(const StringRequest& req, EmptyResponseSender&& send)
{
	auto&& body = req.body();
	if (req[http::field::content_type] == "application/x-www-form-urlencoded")
	{
		auto [username, password, login_from] = find_fields(body, "username", "password", "login-from");

		Authentication::verify_user(
			username,
			Password{password},
			*m_db.alloc(),
			m_cfg.session_length(),
			[
				version=req.version(),
				send=std::move(send),
				login_from=url_decode(login_from),
				session_length=session_length()
			](std::error_code ec, auto&& session) mutable
			{
				Log(LOG_INFO, "%4% login result: %1% %2% (from %3%)", ec, ec.message(), login_from, session.user());

				auto login_incorrect = URLIntent{URLIntent::Action::lib, "", "", "login_incorrect.html"}.str();

				// we want to redirect people to the page they login from. e.g. when they press the
				// login button from /view/user/collection, we want to redirect them to
				// /view/user/collection after they login successfully.
				// Except when they login from /login_incorrect.html: even if they login from that page
				// we won't redirect them back there, because it would look like the login failed.
				if (login_from == login_incorrect)
					login_from.clear();

				auto&& res = see_other(ec ? login_incorrect : (login_from.empty() ? "/" : login_from), version);
				if (!ec)
					res.set(http::field::set_cookie, session.set_cookie(session_length));

				res.set(http::field::cache_control, "no-cache, no-store, must-revalidate");
				send(std::move(res));
			}
		);
	}
	else
		send(see_other("/", req.version()));
}

void Server::on_logout(const EmptyRequest& req, EmptyResponseSender&& send, const Authentication& auth)
{
	auth.destroy_session(*m_db.alloc(), [this, send=std::move(send), version=req.version()](auto&& ec) mutable
	{
		auto&& res = see_other("/", version);
		res.set(http::field::set_cookie, Authentication{}.set_cookie());
		res.set(http::field::cache_control, "no-cache, no-store, must-revalidate");
		res.keep_alive(false);
		send(std::move(res));
	});
}

/// Helper function to create a 303 See Other response.
/// See [MDN](https://developer.mozilla.org/en-US/docs/Web/HTTP/Status/303) for details.
/// It is used to redirect to home page after login, for example, and other cases which
/// we don't want the browser to cache.
http::response<http::empty_body> Server::see_other(boost::beast::string_view where, unsigned version)
{
	http::response<http::empty_body> res{http::status::see_other, version};
	res.set(http::field::location, where);
	return res;
}

void Server::unlink(BlobRequest&& req, EmptyResponseSender&& send)
{
	assert(req.blob());
	Log(LOG_INFO, "unlinking object %1% from path(%2%)", *req.blob(), req.collection());

	if (!req.request_by_owner())
		
		return send(http::response<http::empty_body>{http::status::forbidden, req.version()});

	// remove from user's container
	Ownership{req.owner()}.unlink(
		*m_db.alloc(), req.collection(), *req.blob(),
		[send = std::move(send), version=req.version()](auto ec)
		{
			auto status = http::status::no_content;
			if (ec == Error::object_not_exist)
				status = http::status::bad_request;
			else if (ec)
				status = http::status::internal_server_error;

			return send(http::response<http::empty_body>{status,version});
		}
	);
}

void Server::update_blob(BlobRequest&& req, EmptyResponseSender&& send)
{
	if (!req.request_by_owner())
		return send(http::response<http::empty_body>{http::status::forbidden, req.version()});

	Log(LOG_NOTICE, "receving form string for updating blob %1%: %2%", *req.blob(), req.body());

	assert(req.blob());
	auto [perm_str] = find_fields(req.body(), "perm");
	Log(LOG_NOTICE, "updating blob %1% to %2%", *req.blob(), perm_str);

	auto perm = Permission::from_description(perm_str);

	Ownership{req.owner()}.set_permission(
		*m_db.alloc(), req.collection(), *req.blob(), perm, [send=std::move(send), version=req.version()](auto&& ec)
		{
			send(http::response<http::empty_body>{
				ec ? http::status::internal_server_error : http::status::no_content,
				version
			});
		}
	);
}

void Server::on_upload(UploadRequest&& req, EmptyResponseSender&& send, const Authentication& auth)
{
	boost::system::error_code bec;

	URLIntent path_url{req.target()};
	if (auth.user() != path_url.user())
	{
		// Introduce a small delay when responsing to requests with invalid session ID.
		// This is to slow down bruce-force attacks on the session ID.
		boost::asio::deadline_timer t{m_ioc, boost::posix_time::milliseconds{500}};
		return t.async_wait([version=req.version(), send=std::move(send)](auto ec)
		{
			if (!ec)
				Log(LOG_WARNING, "timer error %1% (%2%)", ec, ec.message());

			send(http::response<http::empty_body>{http::status::forbidden, version});
		});
	}

	Log(LOG_INFO, "uploading %1% bytes to path(%2%) file(%3%)", req.body().size(bec), path_url.collection(), path_url.filename());

	std::error_code ec;
	auto blob = m_blob_db.save(std::move(req.body()), path_url.filename(), ec);
	Log(LOG_INFO, "%5% uploaded %1% to %2% (%3% %4%)", req.target(), blob.ID(), ec, ec.message(), auth.user());

	if (ec)
		return send(http::response<http::empty_body>{http::status::internal_server_error, req.version()});

	// Add the newly created blob to the user's ownership table.
	// The user's ownership table contains all the blobs that is owned by the user.
	// It will be used for authorizing the user's request on these blob later.
	Ownership{auth.user()}.link(
		*m_db.alloc(), path_url.collection(), blob.ID(), blob.entry(), [
			location = URLIntent{URLIntent::Action::blob, auth.user(), path_url.collection(), to_hex(blob.ID())}.str(),
			send = std::move(send),
			version = req.version()
		](auto ec)
		{
			http::response<http::empty_body> res{
				ec ? http::status::internal_server_error : http::status::created,
				version
			};
			if (!ec)
				res.set(http::field::location, location);
			res.set(http::field::cache_control, "no-cache, no-store, must-revalidate");
			return send(std::move(res));
		}
	);
}

http::response<http::string_body> Server::bad_request(boost::beast::string_view why, unsigned version)
{
	http::response<http::string_body> res{
		std::piecewise_construct,
		std::make_tuple(why),
		std::make_tuple(http::status::bad_request, version)
	};
	res.set(http::field::content_type, "text/html");
	return res;
}

// Returns a not found response
http::response<http::string_body> Server::not_found(boost::string_view target, unsigned version)
{
	using namespace std::literals;
	http::response<http::string_body> res{
		std::piecewise_construct,
		std::make_tuple("The resource '"s + target.to_string() + "' was not found."),
		std::make_tuple(http::status::not_found, version)
	};
	res.set(http::field::content_type, "text/plain");
	return res;
}

http::response<http::string_body> Server::server_error(boost::beast::string_view what, unsigned version)
{
	http::response<http::string_body> res{
		std::piecewise_construct,
		std::make_tuple("An error occurred: '" + what.to_string() + "'"),
		std::make_tuple(http::status::internal_server_error, version)
	};
	res.set(http::field::content_type, "text/plain");
	return res;
}

void Server::serve_view(const EmptyRequest& req, Server::FileResponseSender&& send, const Authentication& auth)
{
	if (req.method() != http::verb::get)
		return send(http::response<SplitBuffers>{http::status::bad_request, req.version()});

	URLIntent path_url{req.target()};
	Ownership{path_url.user()}.serialize(
		*m_db.alloc(),
		auth.user(),
		path_url.collection(),
		[send=std::move(send), version=req.version(), auth, this](auto&& json, auto ec)
	{
		std::ostringstream ss;
		ss  << "var dir = " << json << ";";

		auto res = m_lib.find_dynamic("index.html", version);
		res.body().extra(index_needle, ss.str());
		return send(std::move(res));
	});
}

void Server::serve_home(const EmptyRequest& req, FileResponseSender&& send, const Authentication& auth)
{
	if (req.method() != http::verb::get)
		return send(http::response<SplitBuffers>{http::status::bad_request, req.version()});

	Ownership{auth.user()}.scan_all_collections(
		*m_db.alloc(),
		auth.user(),
		[send=std::move(send), ver=req.version(), this](auto&& colls, auto ec)
		{
			std::ostringstream ss;
			ss << "var dir = ";

			rapidjson::OStreamWrapper osw{ss};
			rapidjson::Writer<rapidjson::OStreamWrapper> writer{osw};
			colls.Accept(writer);

			ss << ";";

			auto res = m_lib.find_dynamic("index.html", ver);
			res.body().extra(index_needle, ss.str());
			return send(std::move(res));
		}
	);

}

http::response<SplitBuffers> Server::static_file_request(const URLIntent& intent, boost::string_view etag, unsigned version)
{
	if (intent.filename() == "login_incorrect.html")
	{
		auto res = m_lib.find_dynamic("index.html", version);
		res.body().extra(
			index_needle,
			R"_(var dir = {login_message: "Login incorrect... Try again?"};)_"
		);
		return res;
	}
	else
		return m_lib.find_static(intent.filename(), etag, version);
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

void Server::drop_privileges() const
{
	// drop privileges if run as root
	if (::getuid() == 0)
	{
		// must set group ID before setting user ID, otherwise we have no
		// priviledge to set group ID
		if (::setgid(m_cfg.group_id()) != 0)
			BOOST_THROW_EXCEPTION(hrb::SystemError()
				<< ErrorCode(std::error_code(errno, std::system_category()))
				<< boost::errinfo_api_function("setgid")
			);

		if (::setuid(m_cfg.user_id()) != 0)
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
	Authentication::add_user(username, std::move(password), *m_db.alloc(), [&complete](std::error_code&& ec)
	{
		complete(std::move(ec));
	});
	m_ioc.run();
}

boost::asio::io_context& Server::get_io_context()
{
	return m_ioc;
}

std::string Server::https_root() const
{
	using namespace std::literals;
	return "https://" + m_cfg.server_name()
		+ (m_cfg.listen_https().port() == 443 ? ""s : (":"s + std::to_string(m_cfg.listen_https().port())));
}

std::size_t Server::upload_limit() const
{
	return m_cfg.upload_limit();
}

std::chrono::seconds Server::session_length() const
{
	return m_cfg.session_length();
}

void Server::serve_collection(const EmptyRequest& req, StringResponseSender&& send, const Authentication& auth)
{
	if (req.method() != http::verb::get)
		return send(http::response<http::string_body>{http::status::bad_request, req.version()});

	URLIntent path_url{req.target()};
	Ownership{path_url.user()}.serialize(
		*m_db.alloc(),
		auth.user(),
		path_url.collection(),
		[send=std::move(send), version=req.version()](auto&& json, auto ec)
		{
			http::response<http::string_body> res{
				std::piecewise_construct,
				std::make_tuple(std::move(json)),
				std::make_tuple(http::status::ok, version)
			};
			res.set(http::field::content_type, "application/json");
			return send(std::move(res));
		}
	);
}

void Server::scan_collection(const EmptyRequest& req, Server::StringResponseSender&& send, const Authentication& auth)
{
	if (req.method() != http::verb::get)
		return send(http::response<http::string_body>{http::status::bad_request, req.version()});

	URLIntent path_url{req.target()};

	// TODO: allow other users to query another user's shared collections
	if (auth.user() != path_url.user())
		return send(http::response<http::string_body>{http::status::forbidden, req.version()});

	Ownership{path_url.user()}.scan_all_collections(
		*m_db.alloc(),
		auth.user(),
		[send=std::move(send), ver=req.version()](auto&& colls, auto ec)
		{
			std::ostringstream ss;
			rapidjson::OStreamWrapper osw{ss};
			rapidjson::Writer<rapidjson::OStreamWrapper> writer{osw};
			colls.Accept(writer);

			http::response<http::string_body> res{
				std::piecewise_construct,
				std::make_tuple(ss.str()),
				std::make_tuple(http::status::ok, ver)
			};
			res.set(http::field::content_type, "application/json");
			return send(std::move(res));
		}
	);
}

void Server::prepare_upload(UploadFile& result, std::error_code& ec)
{
	// Need to call prepare_upload() before using UploadRequestBody.
	m_blob_db.prepare_upload(result, ec);
	if (ec)
		Log(LOG_WARNING, "error opening file %1%: %2% (%3%)", m_cfg.blob_path(), ec, ec.message());
}

} // end of namespace
