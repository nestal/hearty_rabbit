/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#pragma once

#include <QtNetwork/QNetworkAccessManager>
#include <memory>

namespace hrb {

class LoginResult : public QObject
{
	Q_OBJECT

public:
	LoginResult() = default;
	bool ok() const {return m_ok;}

signals:
	void finished();

private:
	bool m_ok{false};
};

class QtClient
{
public:
	QtClient() = default;

	std::shared_ptr<LoginResult> login(const QString& site, const QString& user, const QString& password);

private:
	QNetworkAccessManager   m_nam;
};

} // end of namespace hrb
