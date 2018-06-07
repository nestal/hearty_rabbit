/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/6/18.
//

#pragma once

#include <QtNetwork/QNetworkAccessManager>

#include "common/ObjectID.hh"

namespace hrb {

class Collection;

class QtClient : public QObject
{
	Q_OBJECT

public:
	QtClient(QObject *parent);

	void login(const QString& host, int port, const QString& username, const QString& password);
	void list_collection(const QString& collection);

	void get_blob(const QString& owner, const QString& collection, const ObjectID& blob, const QString& rendition);

signals:
	void on_login(std::error_code ec);
	void on_list_collection(const Collection& coll);
	void on_get_blob(const ObjectID& blob, const QString& rendition, const QByteArray& data);

private:
	QUrl setup_url(const std::string& target);

private:
	QString m_host;
	int m_port{0};

	QString m_user;

	QNetworkAccessManager   m_nam{this};
};

} // end of namespace hrb
