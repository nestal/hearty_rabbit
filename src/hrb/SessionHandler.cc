/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 4/8/18.
//

#include "SessionHandler.hh"

#include "BlobDatabase.hh"
#include "BlobFile.hh"
#include "BlobRequest.hh"
#include "Ownership.hh"
#include "Ownership.ipp"
#include "UploadFile.hh"
#include "URLIntent.hh"
#include "WebResources.hh"

#include "crypto/Password.hh"
#include "crypto/Authentication.hh"

#include "net/SplitBuffers.hh"
#include "net/MMapResponseBody.hh"

#include "util/Configuration.hh"
#include "util/Escape.hh"
#include "util/FS.hh"
#include "util/Log.hh"

#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>

namespace hrb {

SessionHandler::SessionHandler(
	std::shared_ptr<redis::Connection>&& db,
	WebResources& lib,
	BlobDatabase& blob_db,
	const Configuration& cfg
) :
	m_db{db}, m_lib{lib}, m_blob_db{blob_db}, m_cfg{cfg}
{
}

std::chrono::seconds SessionHandler::session_length() const
{
	return m_cfg.session_length();
}

void SessionHandler::prepare_upload(UploadFile& result, std::error_code& ec)
{
	// Need to call prepare_upload() before using UploadRequestBody.
	m_blob_db.prepare_upload(result, ec);
	if (ec)
		Log(LOG_WARNING, "error opening file %1%: %2% (%3%)", m_cfg.blob_path(), ec, ec.message());
}

void SessionHandler::on_login(const StringRequest& req, EmptyResponseSender&& send)
{
	auto&& body = req.body();
	if (req[http::field::content_type] == "application/x-www-form-urlencoded")
	{
		auto [username, password, login_from] = find_fields(body, "username", "password", "login-from");

		Authentication::verify_user(
			username,
			Password{password},
			*m_db,
			m_cfg.session_length(),
			[
				version=req.version(),
				send=std::move(send),
				login_from=std::string{login_from},
				session_length=session_length()
			](std::error_code ec, auto&& session) mutable
			{
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

void SessionHandler::on_logout(const EmptyRequest& req, EmptyResponseSender&& send)
{
	m_auth.destroy_session(*m_db, [this, send=std::move(send), version=req.version()](auto&& ec) mutable
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
http::response<http::empty_body> SessionHandler::see_other(boost::beast::string_view where, unsigned version)
{
	http::response<http::empty_body> res{http::status::see_other, version};
	res.set(http::field::location, where);
	return res;
}

void SessionHandler::unlink(BlobRequest&& req, EmptyResponseSender&& send)
{
	assert(req.blob());
//	Log(LOG_INFO, "unlinking object %1% from path(%2%)", *req.blob(), req.collection());

	if (!req.request_by_owner())
		return send(http::response<http::empty_body>{http::status::forbidden, req.version()});

	// remove from user's container
	Ownership{req.owner()}.unlink(
		*m_db, req.collection(), *req.blob(),
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

void SessionHandler::update_blob(BlobRequest&& req, EmptyResponseSender&& send)
{
	if (!req.request_by_owner())
		return send(http::response<http::empty_body>{http::status::forbidden, req.version()});

	assert(req.blob());
	auto [perm_str, move_destination] = find_fields(req.body(), "perm", "move");

	auto on_complete = [send = std::move(send), version = req.version()](auto&& ec)
	{
		send(
			http::response<http::empty_body>{
				ec ? http::status::internal_server_error : http::status::no_content,
				version
			}
		);
	};

	if (!perm_str.empty())
	{
		Ownership{req.owner()}.set_permission(
			*m_db,
			req.collection(),
			*req.blob(),
			Permission::from_description(perm_str),
			std::move(on_complete)
		);
	}
	else if (!move_destination.empty())
	{
		Ownership{req.owner()}.move_blob(
			*m_db,
			req.collection(),
			move_destination,
			*req.blob(),
			std::move(on_complete)
		);
	}
}

void SessionHandler::on_upload(UploadRequest&& req, EmptyResponseSender&& send)
{
	boost::system::error_code bec;

	URLIntent path_url{req.target()};
	if (m_auth.user() != path_url.user())
	{
		// TODO: Introduce a small delay when responsing to requests with invalid session ID.
		// This is to slow down bruce-force attacks on the session ID.
		return send(http::response<http::empty_body>{http::status::forbidden, req.version()});
	}

	std::error_code ec;
	auto blob = m_blob_db.save(std::move(req.body()), path_url.filename(), ec);

	if (ec)
		return send(http::response<http::empty_body>{http::status::internal_server_error, req.version()});

	// Add the newly created blob to the user's ownership table.
	// The user's ownership table contains all the blobs that is owned by the user.
	// It will be used for authorizing the user's request on these blob later.
	Ownership{m_auth.user()}.link(
		*m_db, path_url.collection(), blob.ID(), blob.entry(), [
			location = URLIntent{URLIntent::Action::view, m_auth.user(), path_url.collection(), to_hex(blob.ID())}.str(),
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

http::response<http::string_body> SessionHandler::bad_request(boost::beast::string_view why, unsigned version)
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
http::response<SplitBuffers> SessionHandler::not_found(boost::string_view target, unsigned version)
{
	nlohmann::json dir;
	dir.emplace("error_message", "The request resource was not found.");
	dir.emplace("username", std::string{m_auth.user()});

	return m_lib.inject_json(http::status::not_found, dir.dump(), version);
}

http::response<http::string_body> SessionHandler::server_error(boost::beast::string_view what, unsigned version)
{
	http::response<http::string_body> res{
		std::piecewise_construct,
		std::make_tuple("An error occurred: '" + what.to_string() + "'"),
		std::make_tuple(http::status::internal_server_error, version)
	};
	res.set(http::field::content_type, "text/plain");
	return res;
}

http::response<SplitBuffers> SessionHandler::file_request(const URLIntent& intent, boost::string_view etag, unsigned version)
{
	return intent.filename() == "login_incorrect.html" ?
		m_lib.inject_json(http::status::ok, R"_({login_message: "Login incorrect... Try again?"})_", version) :
		m_lib.find_static(intent.filename(), etag, version);
}

} // end of namespace hrb
