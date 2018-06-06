/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#include "MainWindow.hh"
#include "LoginDialog.hh"

#include <QAction>
#include <QFileSystemModel>
#include <QtNetwork/QNetworkReply>

#include <iostream>

namespace hrb {

MainWindow::MainWindow() :
	m_fs_model{new QFileSystemModel{this}}
{
	m_.setupUi(this);
	m_fs_model->setRootPath(QDir::homePath());
	m_fs_model->setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Drives);
	m_.local_fs->setModel(m_fs_model);
	m_.local_fs->setRootIndex(m_fs_model->index(QDir::homePath()));
	m_.local_fs->setCurrentIndex(m_fs_model->index(QDir::currentPath()));

	// show only the first column in file system view
	for (int i = 1; i < m_fs_model->columnCount({}); i++)
		m_.local_fs->setColumnHidden(i, true);

	connect(m_.action_exit, &QAction::triggered, qApp, &QApplication::quit);

	connect(m_.action_login, &QAction::triggered, [this](bool)
	{
		LoginDialog dlg{this};
		if (dlg.exec() == QDialog::Accepted)
			std::cout << "OK!" << std::endl;
	});


	list_hrb();
}

void MainWindow::list_hrb()
{
	auto reply = m_nam.get(QNetworkRequest{QUrl::fromUserInput("https://www.nestal.net/api/nestal/")});
	connect(reply, &QNetworkReply::finished, [reply]
	{
		std::cout << "read reply = " << QString::fromUtf8(reply->readAll()).toStdString() << std::endl;
	});
}

} // end of namespace hrb
