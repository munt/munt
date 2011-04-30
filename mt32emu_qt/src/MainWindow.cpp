/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtGui>

#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	QString s = "Synth 1";
	ui->synthTabs->addTab(&synth1, s);
	//QObject::connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(on_actionAbout_triggered()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionAbout_triggered()
{
	QMessageBox::about(this, "About",
					   "Munt mt32emu-qt\n"
					   "\n"
					   "Version wibble"
					   "\n"
					   "Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev\n"
					   "\n"
					   "Blah blah GPL etc."
					   );
}
