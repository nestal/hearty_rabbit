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

#include "common/URLIntent.hh"

#include <iostream>

namespace hrb {

QtClient::QtClient(QObject *parent) : QObject{parent}
{
#ifndef NDEBUG
	connect(&m_nam, &QNetworkAccessManager::sslErrors, [](QNetworkReply *reply, auto errors){reply->ignoreSslErrors();});
#endif
}

void QtClient::login(const QString& host, int port, const QString& username, const QString& password)
{
	m_host = host;
	m_port = port;

	QNetworkRequest request{setup_url(URLIntent{URLIntent::Action::login, "", "", ""}.str())};
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

	QString post = "username=" + username + "&password=" + password;
	auto reply = m_nam.post(request, post.toUtf8());
	connect(reply, &QNetworkReply::finished, [reply, this]
	{
		std::cout << "read reply = " << reply->errorString().toStdString() << " " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << std::endl;

		if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 204)
			std::cout << "login success" << std::endl;

		emit on_login({});

		reply->deleteLater();
	});
}

QUrl QtClient::setup_url(const std::string& target)
{
	QUrl url;
	url.setScheme("https");
	url.setHost(m_host);
	url.setPort(m_port);
	url.setPath(QString::fromStdString(target));
	return url;
}

} // end of namespace hrb
