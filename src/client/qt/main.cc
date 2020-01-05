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

#include <QApplication>

int main(int argc, char *argv[])
{
//	Q_INIT_RESOURCE(application);

	QApplication app{argc, argv};
	hrb::MainWindow wnd;
	wnd.show();

	return app.exec();
}
