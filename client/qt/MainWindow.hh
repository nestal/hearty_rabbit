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

#include "ui_MainWindow.h"

#include <QtWidgets/QMainWindow>

#include <system_error>

class QFileSystemModel;

namespace hrb {

class QtClient;
class CollectionModel;
class CollectionListModel;

class MainWindow : public QMainWindow
{
public:
	MainWindow();

private:
	void on_login(std::error_code err);

private:
	Ui::MainWindow      m_;
	QtClient            *m_hrb{};

	QString m_username;

	QFileSystemModel    *m_fs_model{};
	CollectionModel     *m_coll_model{};
	CollectionListModel *m_coll_list_model{};
};

} // end of namespace hrb
