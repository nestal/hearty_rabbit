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
#include "Container.hh"
#include "ResourcesList.hh"

#include "crypto/Password.hh"
#include "crypto/Authentication.hh"
#include "net/Listener.hh"
#include "util/Error.hh"
#include "util/Configuration.hh"
#include "util/Exception.hh"
#include "util/Escape.hh"
#include "util/Log.hh"

#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/info.hpp>
#include <boost/filesystem.hpp>

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

namespace hrb {

Server::Server(const Configuration& cfg) :
	m_cfg{cfg},
	m_ioc{static_cast<int>(std::max(1UL, cfg.thread_count()))},
	m_db{m_ioc, cfg.redis()},
	m_lib{cfg.web_root()},
	m_blob_db{cfg.blob_path()}
{
	OpenSSL_add_all_digests();
}

void Server::on_login(const StringRequest& req, EmptyResponseSender&& send)
{
	auto&& body = req.body();
	if (req[http::field::content_type] == "application/x-www-form-urlencoded")
	{
		auto [username, password] = find_fields(body, "username", "password");

		Authentication::verify_user(
			username,
			Password{password},
			*m_db.alloc(),
			[
				version=req.version(),
				send=std::move(send)
			](std::error_code ec, auto&& session) mutable
			{
				Log(LOG_INFO, "login result: %1% %2%", ec, ec.message());

				auto&& res = see_other(ec ? "/login_incorrect.html" : "/", version);
				if (!ec)
					res.set(http::field::set_cookie, session.set_cookie());

				res.set(http::field::cache_control, "no-cache, no-store, must-revalidate");
				send(std::move(res));
			}
		);
	}
	else
		send(see_other("/", req.version()));
}

void Server::on_logout(const EmptyRequest& req, const Authentication& auth, EmptyResponseSender&& send)
{
	auth.destroy_session(*m_db.alloc(), [this, send=std::move(send), version=req.version()](auto&& ec) mutable
	{
		auto&& res = see_other("/", version);
		res.set(http::field::set_cookie, "id=; ");
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

void Server::get_blob(const EmptyRequest& req, BlobResponseSender&& send, const Authentication& auth)
{
	auto blob_id = req.target().size() > url::login.size() ?
		req.target().substr(url::login.size()) :
		boost::string_view{};

	// Return 404 not_found if the blob ID is invalid
	auto object_id = hex_to_object_id(std::string_view{blob_id.data(), blob_id.size()});
	if (object_id == ObjectID{})
		return send(http::response<MMapResponseBody>{http::status::not_found, req.version()});

	// Check if the user has permission to read the blob
	Container::is_member(*m_db.alloc(), auth.user(), object_id,
		[
			send=std::move(send),
			object_id,
			version=req.version(),
			etag=req[http::field::if_none_match].to_string(),
			this
		](auto ec, bool is_member) mutable
		{
			if (ec)
				return send(http::response<MMapResponseBody>{http::status::internal_server_error, version});

			if (!is_member)
				return send(http::response<MMapResponseBody>{http::status::forbidden, version});

			return send(m_blob_db.response(object_id, m_magic, version, etag));
		}
	);
}

void Server::on_upload(UploadRequest&& req, EmptyResponseSender&& send, const Authentication& auth)
{
	boost::system::error_code bec;
	auto [prefix, filename] = extract_prefix(req);

	std::error_code ec;
	auto id = m_blob_db.save(req.body(), ec);
	Log(LOG_INFO, "uploaded %1% bytes to %2% (%3% %4%)", req.body().size(bec), id, ec, ec.message());

	if (ec)
		return send(http::response<http::empty_body>{http::status::internal_server_error, req.version()});

	// Add the newly created blob to the user's container.
	// The user's container contains all the blobs that is owned by the user.
	// It will be used for authorizing the user's request on these blob later.
	Container::add(*m_db.alloc(), auth.user(), id, [send=std::move(send), id, version=req.version(), this](auto ec) mutable
	{
		http::response<http::empty_body> res{
			ec ? http::status::internal_server_error : http::status::created,
			version
		};
		if (!ec)
			res.set(http::field::location, "/blob/" + to_hex(id));
		res.set(http::field::cache_control, "no-cache, no-store, must-revalidate");
		send(std::move(res));
	});
}

void Server::on_invalid_session(const RequestHeader& req, FileResponseSender&& send)
{
	// If the target is home (i.e. "/"), show them the login page.
	// Do not penalize the user, because their session may be expired.
	// It may also be the first visit of an anonymous user.
	if (req.target() == "/")
		return send(m_lib.find_dynamic("login.html", req.version()));

	// Introduce a small delay when responsing to requests with invalid session ID.
	// This is to slow down bruce-force attacks on the session ID.
	boost::asio::deadline_timer t{m_ioc, boost::posix_time::milliseconds{500}};
	t.async_wait([version=req.version(), send=std::move(send), target=req.target().to_string()](auto ec)
	{
		if (!ec)
			Log(LOG_WARNING, "timer error %1% (%2%)", ec, ec.message());

		send(http::response<SplitBuffers>{http::status::forbidden, version});
	});
}

http::response<http::string_body> Server::get_dir(const EmptyRequest& req)
{
	return http::response<http::string_body>();
}

http::response<http::string_body> Server::bad_request(boost::beast::string_view why, unsigned version)
{
	http::response<http::string_body> res{
		std::piecewise_construct,
		std::make_tuple(why),
		std::make_tuple(http::status::bad_request, version)
	};
	res.set(http::field::content_type, "text/html");
	res.prepare_payload();
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
	res.prepare_payload();
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
	res.prepare_payload();
	return res;
}

void Server::serve_home(FileResponseSender&& send, unsigned version, const Authentication& auth)
{
	Container::load(*m_db.alloc(), auth.user(), [send=std::move(send), version, this](auto ec, Container&& cond)
	{
		std::ostringstream script_tag;
		script_tag << "<script>var dir = ";

		rapidjson::OStreamWrapper osw{script_tag};
		rapidjson::Writer<rapidjson::OStreamWrapper> writer{osw};
		cond.serialize().Accept(writer);
		script_tag << ";</script>";

		auto res = m_lib.find_dynamic("index.html", version);
		res.body().extra("<meta charset=\"utf-8\">", script_tag.str());
		send(std::move(res));
	});
}

http::response<SplitBuffers> Server::static_file_request(const EmptyRequest& req)
{
	Log(LOG_NOTICE, "requesting path %1%", req.target());

	auto filepath = req.target();
	filepath.remove_prefix(1);

	return m_lib.find_static(std::string{filepath}, req.version());
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

bool Server::allow_anonymous(boost::string_view target)
{
	assert(!target.empty());
	assert(target.front() == '/');
	target.remove_prefix(1);

	return static_resources.find(target.to_string()) != static_resources.end();
}

std::tuple<
	std::string_view,
	std::string_view
> Server::extract_prefix(const RequestHeader& req)
{
	auto target = req.target();
	auto sv = std::string_view{target.data(), target.size()};
	if (!sv.empty() && sv.front() == '/')
		sv.remove_prefix(1);

	auto prefix = std::get<0>(split_front(sv, "/?$"));
	return std::make_tuple(prefix, sv);
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

/// \arg    header      The header we just received. This reference must be valid
///						until \a complete() is called.
/// \arg    src         The request_parser that produce \a header. It will be moved
///                     to \a dest. Must be valid until \a complete() is called.
void Server::on_request_header(
	const RequestHeader& header,
	EmptyRequestParser& src,
	RequestBodyParsers& dest,
	std::function<void(const Authentication&)>&& complete
)
{
	// Use StringRequestParser to parser login requests.
	// The username/password will be stored in the string body.
	// No need to verify session.
	if (is_login(header))
	{
		dest.emplace<StringRequestParser>(std::move(src));
		return complete(Authentication{});
	}

	if (allow_anonymous(header.target()))
	{
		dest.emplace<EmptyRequestParser>(std::move(src));
		return complete(Authentication{});
	}

	// Everything else require a valid session.
	auto cookie = header[http::field::cookie];
	auto session = parse_cookie({cookie.data(), cookie.size()});
	if (!session)
	{
		dest.emplace<EmptyRequestParser>(std::move(src));
		return complete(Authentication{});
	}

	Authentication::verify_session(
		*session,
		*m_db.alloc(),
		[
			this,
			&header,
			&dest,
			&src,
			complete=std::move(complete)
		](std::error_code ec, const Authentication& auth) mutable
		{
			// Use a UploadRequestParse to parser upload requests, only when the session is authenicated.
			if (!ec && is_upload(header))
			{
				// Need to call prepare_upload() before using UploadRequestBody.
				m_blob_db.prepare_upload(dest.emplace<UploadRequestParser>(std::move(src)).get().body(), ec);
				if (ec)
					Log(LOG_WARNING, "error opening file %1%: %2% (%3%)", m_cfg.blob_path(), ec, ec.message());
			}

			// Other requests use EmptyRequestParser, because they don't have a body.
			else
			{
				dest.emplace<EmptyRequestParser>(std::move(src));
			}

			complete(auth);
		}
	);
}

bool Server::is_upload(const RequestHeader& header)
{
	return header.target().starts_with(hrb::url::upload) && header.method() == http::verb::put;
}

bool Server::is_login(const RequestHeader& header)
{
	return header.target() == hrb::url::login && header.method() == http::verb::post;
}

} // end of namespace
