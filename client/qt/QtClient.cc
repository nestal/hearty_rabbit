/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/6/18.
//

#include "QtClient.hh"

#include <QtNetwork/QNetworkReply>

#include "common/Collection.hh"
#include "common/CollectionList.hh"
#include "common/Escape.hh"
#include "common/URLIntent.hh"

#include <nlohmann/json.hpp>
#include <iostream>

namespace hrb {

QtClient::QtClient(QObject *parent) : QObject{parent}
{
#ifndef NDEBUG
	// ignore SSL error in debug mode to allow easy testing
	connect(&m_nam, &QNetworkAccessManager::sslErrors, [](QNetworkReply *reply, auto errors){reply->ignoreSslErrors();});
#endif
}

void QtClient::login(const QString& host, int port, const QString& username, const QString& password)
{
	m_host = host;
	m_port = port;
	m_user = username;

	QNetworkRequest request{setup_url({URLIntent::Action::login, "", "", ""})};
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

	QString post = "username=" + username + "&password=" + password;
	auto reply = m_nam.post(request, post.toUtf8());
	connect(reply, &QNetworkReply::finished, [reply, this]
	{
		std::cout << "read reply = " << reply->errorString().toStdString() << " " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << std::endl;

		if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 204)
			std::cout << "login success" << std::endl;

		Q_EMIT on_login({});

		reply->deleteLater();
	});
}

QUrl QtClient::setup_url(const URLIntent& intent)
{
	QUrl url;
	url.setScheme("https");
	url.setHost(m_host);
	url.setPort(m_port);
	url.setPath(QString::fromStdString(intent.path()));
	url.setQuery(QString::fromStdString(std::string{intent.option()}));
	return url;
}

void QtClient::list_collection(const QString& collection)
{
	QNetworkRequest request{setup_url({
		URLIntent::Action::api,
		m_user.toStdString(),
		collection.toStdString(),
		""
	})};

	auto reply = m_nam.get(request);
	connect(reply, &QNetworkReply::finished, [reply, this]
	{
		auto dir = nlohmann::json::parse(reply->readAll().toStdString(), nullptr, false);
		if (!dir.is_discarded())
			Q_EMIT on_list_collection(dir.get<Collection>());
		else
			std::cout << "parse error!" << std::endl;
	});
}

void QtClient::get_blob(const QString& owner, const QString& collection, const ObjectID& blob, const QString& rendition)
{
	QNetworkRequest request{setup_url({
		URLIntent::Action::api,
		owner.toStdString(),
		collection.toStdString(),
		to_hex(blob),
		"rendition=" + rendition.toStdString()
	})};

	auto reply = m_nam.get(request);
	connect(reply, &QNetworkReply::finished, [reply, this, blob, rendition]
	{
		if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
			Q_EMIT on_get_blob(blob, rendition, reply->readAll());
	});
}

void QtClient::owned_collections(const QString& user)
{
	QNetworkRequest request{setup_url({URLIntent::QueryTarget::collection, "json&user=" + user.toStdString()})};

	auto reply = m_nam.get(request);
	connect(reply, &QNetworkReply::finished, [reply, this]
	{
		if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
		{
			std::cout
				<< reply->errorString().toStdString()
				<< " "
				<< reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
				<< std::endl;

			auto json = reply->readAll();
			std::cout << json.toStdString() << std::endl;

			auto dir = nlohmann::json::parse(json.toStdString(), nullptr, false);
			Q_EMIT on_owned_collections(dir);
		}
	});
}

} // end of namespace hrb
