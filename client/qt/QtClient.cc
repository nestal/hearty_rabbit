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
}

void QtClient::login(const QString& site, const QString& username, const QString& password)
{
	URLIntent login_url{URLIntent::Action::login, "", "", ""};
	QString post = "username=" + username + "&password=" + password;

	auto reply = m_nam.post(
		QNetworkRequest{QUrl::fromUserInput(QString::fromStdString(login_url.str()))},
		post.toUtf8()
	);
	connect(reply, &QNetworkReply::finished, [reply, this]
	{
		//if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 204)
		std::cout << "read reply = " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString().toStdString() << std::endl;
		emit on_login({});
	});
}

} // end of namespace hrb
