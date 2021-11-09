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
#include "WebResources.hh"

#include "hrb/BlobInode.hh"
#include "hrb/Collection.hh"
#include "hrb/URLIntent.hh"
#include "util/StringFields.hh"

#include "crypto/Password.hh"
#include "crypto/Authentication.hh"
#include "crypto/Authentication.ipp"

#include "net/SplitBuffers.hh"
#include "net/MMapResponseBody.hh"

#include "util/Configuration.hh"
#include "util/Escape.hh"
#include "util/FS.hh"
#include "util/Log.hh"
#include "hrb/index/PHashDb.hh"

#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>

namespace hrb {

SessionHandler::SessionHandler(
	std::shared_ptr<redis::Connection>&& db,
	WebResources& lib,
//	BlobDatabase& blob_db,
	const Configuration& cfg
) :
	m_lib{lib}, m_server{cfg.blob_path(), std::move(db), m_cfg.session_length()}, m_cfg{cfg}
{
	assert(!m_server.auth().is_valid());
}

std::chrono::seconds SessionHandler::session_length() const
{
	return m_cfg.session_length();
}

void SessionHandler::prepare_upload(UploadFile& result, std::error_code& ec)
{
	boost::system::error_code err;
	result.open(m_cfg.blob_path(), err);
	if (err)
		ec.assign(err.value(), err.category());

//	m_blob_db.prepare_upload(result, ec);
	if (ec)
		Log(LOG_WARNING, "error opening file %1%: %2% (%3%)", m_cfg.blob_path(), ec, ec.message());
}

void SessionHandler::on_login(const StringRequest& req, EmptyResponseSender&& send)
{
	auto&& body = req.body();
	if (req[http::field::content_type] == "application/x-www-form-urlencoded")
	{
		auto [username, password] = urlform.find(body, "username", "password");

		assert(!m_server.auth().is_valid());

		m_server.login(
			username,
			Password{password},
			[
				this,
				version=req.version(),
				send=std::move(send)
			](std::error_code ec) mutable
			{
				assert(m_server.auth().invariance());

				http::response<http::empty_body> res{
					ec == Error::login_incorrect ?
						http::status::forbidden :
						(ec ? http::status::internal_server_error : http::status::no_content),
					version
				};

				res.set(http::field::cache_control, "no-cache, no-store, must-revalidate");
				send(std::move(res));
			}
		);
	}
	else
		send(http::response<http::empty_body>{http::status::bad_request, req.version()});
}

void SessionHandler::on_logout(const EmptyRequest& req, EmptyResponseSender&& send)
{
	m_server.destroy_session([this, send=std::move(send), version=req.version()](auto&& ec) mutable
	{
		// clear the user credential.
		// Session will set the cookie in the response so no need to set here
		assert(!m_server.auth().is_valid());

		http::response<http::empty_body> res{http::status::ok, version};
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
//	assert(req.blob());
//	if (!req.request_by_owner(m_server.auth().id()))
//		return send(http::response<http::empty_body>{http::status::forbidden, req.version()});

	// remove from user's container
//	Ownership{req.owner(), m_server.auth().id()}.unlink_blob(
//		*m_db, req.collection(), *req.blob(),
//		[send = std::move(send), version = req.version()](auto ec)
//		{
//			auto status = http::status::no_content;
//			if (ec == Error::object_not_exist)
//				status = http::status::bad_request;
//			else if (ec)
//				status = http::status::internal_server_error;
//
//			return send(http::response<http::empty_body>{status, version});
//		}
//	);
}

void SessionHandler::post_blob(BlobRequest&& req, EmptyResponseSender&& send)
{
	// Only owners can change blob permission
//	if (!req.request_by_owner(m_server.auth().id()))
//		return send(http::response<http::empty_body>{http::status::forbidden, req.version()});

//	assert(req.blob());
	auto [perm_str, move_destination] = urlform.find(req.body(), "perm", "move");

	if (perm_str.empty() && move_destination.empty())
		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});

	auto on_complete = [send = std::move(send), version = req.version()](auto&& ec)
	{
		send(
			http::response<http::empty_body>{
				ec ? http::status::internal_server_error : http::status::no_content,
				version
			}
		);
	};
/*
	if (!perm_str.empty())
		Ownership{req.owner(), m_auth.id()}.set_permission(
			*m_db,
			*req.blob(),
			Permission::from_description(perm_str),
			std::move(on_complete)
		);

	else if (!move_destination.empty())
		Ownership{req.owner(), m_auth.id()}.move_blob(
			*m_db,
			req.collection(),
			move_destination,
			*req.blob(),
			std::move(on_complete)
		);*/
}

void SessionHandler::on_upload(UploadRequest&& req, EmptyResponseSender&& send)
{
	boost::system::error_code bec;

	URLIntent path_url{req.target()};
//	if (m_server.auth().id().username() != path_url.user())
	{
		// TODO: Introduce a small delay when responsing to requests with invalid session ID.
		// This is to slow down bruce-force attacks on the session ID.
		return send(http::response<http::empty_body>{http::status::forbidden, req.version()});
	}

	// Reject empty file
	boost::system::error_code err;
	auto upload_size = req.body().size(err);
	if (err)
		return send(http::response<http::empty_body>{http::status::internal_server_error, req.version()});

	else if (upload_size == 0)
		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});

	std::error_code ec;
//	auto blob = m_blob_db.save(std::move(req.body()), ec);
	std::filesystem::directory_entry upload_file{req.body().path(), ec};
	if (ec)
		return send(http::response<http::empty_body>{http::status::internal_server_error, req.version()});

//	m_server.upload_file(upload_file, path_url.collection(), [
//			location = URLIntent{
//				URLIntent::Action::api,
//				m_server.auth().id().username(),
//				path_url.collection(),
//				path_url.filename()
//			}.str(),
//			send = std::move(send),
//			version = req.version()
//		](auto ec)
//		{
//			http::response<http::empty_body> res{
//				ec ? http::status::internal_server_error : http::status::created,
//				version
//			};
//			if (!ec)
//				res.set(http::field::location, location);
//			res.set(http::field::cache_control, "no-cache, no-store, must-revalidate");
//			return send(std::move(res));
//		}
//	);
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
http::response<SplitBuffers> SessionHandler::not_found(std::string_view target, unsigned version)
{
	nlohmann::json dir;
	dir.emplace("error_message", "The request resource was not found.");
	dir.emplace("username", std::string{m_server.auth().id().username()});

	return m_lib.inject(http::status::not_found, dir.dump(), "<meta></meta>", version);
}

http::response<http::string_body> SessionHandler::server_error(std::string_view what, unsigned version)
{
	std::string msg = "An error occurred: '";
	msg.append(what).push_back('\'');

	http::response<http::string_body> res{
		std::piecewise_construct,
		std::make_tuple(msg),
		std::make_tuple(http::status::internal_server_error, version)
	};
	res.set(http::field::content_type, "text/plain");
	return res;
}

http::response<SplitBuffers> SessionHandler::file_request(const URLIntent& intent, std::string_view etag, unsigned version)
{
	assert(intent.lib());
	return intent.lib()->filename == "login_incorrect.html" ?
		m_lib.inject(http::status::ok, R"_({login_message: "Login incorrect... Try again?"})_", "", version) :
		m_lib.find_static(intent.lib()->filename, etag, version);
}

bool SessionHandler::renewed_auth() const
{
	assert(m_server.auth().invariance());

	return m_request_session_id.has_value()      ?      // if request cookie is present, check against the new cookie
		*m_request_session_id != m_server.auth().session() :     // otherwise, there's no cookie in the original request,
		m_server.auth().id().is_valid();                         // if we have a valid (non-guest) cookie now,
														// then the session is renewed.
}

std::string SessionHandler::server_root() const
{
	return m_cfg.https_root();
}

void SessionHandler::validate_collection(Collection& coll)
{
	for (auto& [id, entry] : coll)
	{
		if (entry.timestamp == Timestamp{})
		{
			Log(LOG_WARNING, "%1% has no timestamp in collection entry: loading from disk", to_hex(id));

//			auto blob_file = m_blob_db.find(id);

//			auto new_entry{entry};
//			new_entry.timestamp = blob_file.original_datetime();
//			coll.update_timestamp(id, new_entry.timestamp);

//			Ownership{m_auth.id()}.update_blob(*m_db, id, new_entry);
		}
	}
}

void SessionHandler::set_header(const BlobInode& entry, boost::beast::http::fields& header)
{
	header.set(http::field::content_disposition, "inline; filename=" + url_encode(entry.filename));
	header.set(http::field::last_modified, entry.timestamp.http_format());

	if (header.find(http::field::content_type) == header.end())
		header.set(http::field::content_type, entry.mime);
}

} // end of namespace hrb
