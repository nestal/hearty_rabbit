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
#include <QAction>

namespace hrb {

MainWindow::MainWindow()
{
	m_.setupUi(this);

	connect(m_.m_action_exit, &QAction::triggered, []{QApplication::quit();});
}

} // end of namespace hrb
