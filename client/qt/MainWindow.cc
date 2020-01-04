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

#include "CollectionModel.hh"
#include "CollectionListModel.hh"
#include "LoginDialog.hh"
#include "QtClient.hh"

#include <QAction>
#include <QFileSystemModel>

#include <iostream>

namespace hrb {

MainWindow::MainWindow() :
	m_hrb{new QtClient{this}},
	m_fs_model{new QFileSystemModel{this}},
	m_coll_model{new CollectionModel{this, m_hrb}},
	m_coll_list_model{new CollectionListModel{this, m_hrb}}
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

	connect(m_hrb, &QtClient::on_login, this, &MainWindow::on_login);
	connect(m_hrb, &QtClient::on_list_collection, m_coll_model, &CollectionModel::update);
	m_.remote_list->setModel(m_coll_model);

	connect(m_hrb, &QtClient::on_owned_collections, m_coll_list_model, &CollectionListModel::update);
	m_.remote_dirs->setModel(m_coll_list_model);

	connect(m_.remote_dirs, &QAbstractItemView::clicked, [this](const QModelIndex& item)
	{
		auto entry = m_coll_list_model->find(item);
		m_hrb->list_collection(QString::fromUtf8(entry.name().data(), entry.name().size()));
	});

	connect(m_.action_login, &QAction::triggered, [this](bool)
	{
		LoginDialog dlg{this};
		if (dlg.exec() == QDialog::Accepted)
		{
			m_username = dlg.username();

			m_hrb->login(dlg.host(), dlg.port(), m_username, dlg.password());
		}
	});
	connect(m_.action_exit, &QAction::triggered, qApp, &QApplication::quit);
}

void MainWindow::on_login(std::error_code err)
{
	std::cout << "login!" << std::endl;

	m_hrb->list_collection("");
	m_hrb->owned_collections(m_username);
}

} // end of namespace hrb
